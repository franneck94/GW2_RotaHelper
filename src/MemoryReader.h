#pragma once

#include <Windows.h>
#include <cstdint>
#include <string>
#include <vector>

struct SkillCastInfo
{
    uint32_t skill_id;
    std::string skill_name;
    float cast_time_remaining;
    float total_cast_time;
    bool is_casting;
    bool is_channeling;
};

class GW2MemoryReader
{
public:
    GW2MemoryReader();
    ~GW2MemoryReader();

    bool Initialize();
    void Cleanup();

    SkillCastInfo GetCurrentSkillCast();
    bool IsSkillCasting();

    // Memory pattern scanning
    uintptr_t FindPattern(const char *pattern, const char *mask, uintptr_t start = 0, size_t size = 0);

    // Memory reading utilities
    template <typename T>
    T ReadMemory(uintptr_t address);

    bool ReadMemoryBuffer(uintptr_t address, void *buffer, size_t size);
    std::string ReadString(uintptr_t address, size_t max_length = 256);

private:
    HANDLE m_process_handle;
    uintptr_t m_base_address;
    size_t m_module_size;

    // Memory offsets (these need to be found through reverse engineering)
    struct MemoryOffsets
    {
        uintptr_t skill_cast_base = 0x0;
        uintptr_t current_skill_id = 0x0;
        uintptr_t cast_time_remaining = 0x0;
        uintptr_t total_cast_time = 0x0;
        uintptr_t is_casting_flag = 0x0;
        uintptr_t skill_name_ptr = 0x0;
    };

    MemoryOffsets m_offsets;

    struct Patterns
    {
        // Pattern for skill casting state
        const char *skill_cast_pattern = "\x8B\x0D\x00\x00\x00\x00\x85\xC9\x74\x00\x8B\x81";
        const char *skill_cast_mask = "xx????xxx?xx";

        // Pattern for skill ID
        const char *skill_id_pattern = "\x89\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x85\xC0";
        const char *skill_id_mask = "xx????x????xx";

        // Pattern for cast time
        const char *cast_time_pattern = "\xF3\x0F\x11\x05\x00\x00\x00\x00\xF3\x0F\x10\x05";
        const char *cast_time_mask = "xxxx????xxxx";
    };

    Patterns m_patterns;

    bool FindMemoryOffsets();
    bool ValidateOffsets();
    uintptr_t GetModuleBaseAddress();
    size_t GetModuleSize();
};

// Template implementation
template <typename T>
T GW2MemoryReader::ReadMemory(uintptr_t address)
{
    T value = {};
    ReadMemoryBuffer(address, &value, sizeof(T));
    return value;
}
