#include "MemoryReader.h"
#include <TlHelp32.h>
#include <Psapi.h>
#include <iostream>

#pragma comment(lib, "psapi.lib")

GW2MemoryReader::GW2MemoryReader()
    : m_process_handle(nullptr)
    , m_base_address(0)
    , m_module_size(0)
{
}

GW2MemoryReader::~GW2MemoryReader()
{
    Cleanup();
}

bool GW2MemoryReader::Initialize()
{
    m_process_handle = GetCurrentProcess();

    if (!m_process_handle)
    {
        return false;
    }

    m_base_address = GetModuleBaseAddress();
    m_module_size = GetModuleSize();

    if (m_base_address == 0 || m_module_size == 0)
        return false;

    if (!FindMemoryOffsets())
        return false;

    return ValidateOffsets();
}

void GW2MemoryReader::Cleanup()
{
    // No need to close GetCurrentProcess() handle
    m_process_handle = nullptr;
    m_base_address = 0;
    m_module_size = 0;
}

SkillCastInfo GW2MemoryReader::GetCurrentSkillCast()
{
    SkillCastInfo info = {};

    if (m_offsets.skill_cast_base == 0)
        return info;

    try
    {
        if (m_offsets.is_casting_flag != 0)
            info.is_casting = ReadMemory<bool>(m_offsets.is_casting_flag);

        if (!info.is_casting)
            return info;

        if (m_offsets.current_skill_id != 0)
            info.skill_id = ReadMemory<uint32_t>(m_offsets.current_skill_id);

        if (m_offsets.cast_time_remaining != 0)
            info.cast_time_remaining = ReadMemory<float>(m_offsets.cast_time_remaining);

        if (m_offsets.total_cast_time != 0)
            info.total_cast_time = ReadMemory<float>(m_offsets.total_cast_time);

        if (m_offsets.skill_name_ptr != 0)
        {
            uintptr_t name_ptr = ReadMemory<uintptr_t>(m_offsets.skill_name_ptr);
            if (name_ptr != 0)
                info.skill_name = ReadString(name_ptr, 128);
        }

        // Determine if it's channeling (cast time > certain threshold)
        info.is_channeling = info.total_cast_time > 1.0f;
    }
    catch (...)
    {
        // Reset info on any exception
        info = {};
    }

    return info;
}

bool GW2MemoryReader::IsSkillCasting()
{
    if (m_offsets.is_casting_flag == 0)
        return false;

    try
    {
        return ReadMemory<bool>(m_offsets.is_casting_flag);
    }
    catch (...)
    {
        return false;
    }
}

uintptr_t GW2MemoryReader::FindPattern(const char* pattern, const char* mask, uintptr_t start, size_t size)
{
    if (start == 0)
        start = m_base_address;

    if (size == 0)
        size = m_module_size;

    size_t pattern_length = strlen(mask);

    for (uintptr_t i = start; i < start + size - pattern_length; ++i)
    {
        bool found = true;
        for (size_t j = 0; j < pattern_length; ++j)
        {
            if (mask[j] == 'x')
            {
                uint8_t memory_byte = ReadMemory<uint8_t>(i + j);
                if (memory_byte != static_cast<uint8_t>(pattern[j]))
                {
                    found = false;
                    break;
                }
            }
        }

        if (found)
            return i;
    }

    return 0;
}

bool GW2MemoryReader::ReadMemoryBuffer(uintptr_t address, void* buffer, size_t size)
{
    if (!m_process_handle || !buffer || size == 0)
        return false;

    SIZE_T bytes_read = 0;
    bool result = ReadProcessMemory(m_process_handle,
                                   reinterpret_cast<LPCVOID>(address),
                                   buffer,
                                   size,
                                   &bytes_read);

    return result && bytes_read == size;
}

std::string GW2MemoryReader::ReadString(uintptr_t address, size_t max_length)
{
    std::vector<char> buffer(max_length + 1, 0);

    if (!ReadMemoryBuffer(address, buffer.data(), max_length))
        return "";

    buffer[max_length] = '\0';  // Ensure null termination
    return std::string(buffer.data());
}

bool GW2MemoryReader::FindMemoryOffsets()
{
    // Find skill casting state pattern
    auto skill_cast_addr = FindPattern(m_patterns.skill_cast_pattern,
                                           m_patterns.skill_cast_mask);

    if (skill_cast_addr != 0)
    {
        // Extract the offset from the found instruction
        // This assumes the pattern is something like: mov ecx, [address]
        auto offset_addr = skill_cast_addr + 2; // Skip the mov opcode
        m_offsets.is_casting_flag = ReadMemory<uint32_t>(offset_addr);
    }

    // Find skill ID pattern
    auto skill_id_addr = FindPattern(m_patterns.skill_id_pattern,
                                         m_patterns.skill_id_mask);

    if (skill_id_addr != 0)
    {
        auto offset_addr = skill_id_addr + 2;
        m_offsets.current_skill_id = ReadMemory<uint32_t>(offset_addr);
    }

    // Find cast time pattern
    auto cast_time_addr = FindPattern(m_patterns.cast_time_pattern,
                                          m_patterns.cast_time_mask);

    if (cast_time_addr != 0)
    {
        auto offset_addr = cast_time_addr + 4;
        m_offsets.cast_time_remaining = ReadMemory<uint32_t>(offset_addr);

        // Assume total cast time is nearby (this needs verification)
        m_offsets.total_cast_time = m_offsets.cast_time_remaining + 4;
    }

    // Set skill cast base (use the casting flag as base for now)
    m_offsets.skill_cast_base = m_offsets.is_casting_flag;

    return m_offsets.skill_cast_base != 0;
}

bool GW2MemoryReader::ValidateOffsets()
{
    // Basic validation - ensure we have at least the casting flag
    if (m_offsets.is_casting_flag == 0)
        return false;

    try
    {
        // Try to read from the casting flag address to validate it's accessible
        ReadMemory<bool>(m_offsets.is_casting_flag);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

uintptr_t GW2MemoryReader::GetModuleBaseAddress()
{
    HMODULE hModule = GetModuleHandle(nullptr);
    return reinterpret_cast<uintptr_t>(hModule);
}

size_t GW2MemoryReader::GetModuleSize()
{
    HMODULE hModule = GetModuleHandle(nullptr);
    if (!hModule)
        return 0;

    MODULEINFO module_info = {};
    if (GetModuleInformation(GetCurrentProcess(), hModule, &module_info, sizeof(module_info)))
        return module_info.SizeOfImage;

    return 0;
}
