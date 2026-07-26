#include <algorithm>
#include <iterator>

#include "KeyboardCapture.h"

namespace
{
constexpr size_t MAX_PRESS_QUEUE = 128;
}

bool KeyboardCapture::Initialize(void (*wndprocRegister)(UINT (*)(HWND, UINT, WPARAM, LPARAM)),
                                 void (*wndprocDeregister)(UINT (*)(HWND, UINT, WPARAM, LPARAM)))
{
    if (m_IsInitialized)
        return true;
    if (!wndprocRegister || !wndprocDeregister)
        return false;

    m_WndProcDeregister = wndprocDeregister;
    m_IsInitialized = true;
    wndprocRegister(NexusWndProcCallback);
    return true;
}

void KeyboardCapture::Shutdown()
{
    if (!m_IsInitialized)
        return;

    if (m_WndProcDeregister)
        m_WndProcDeregister(NexusWndProcCallback);

    {
        std::lock_guard<std::mutex> lock(m_KeyStateMutex);
        m_KeyDown.clear();
        m_PressQueue.clear();
    }

    m_WndProcDeregister = nullptr;
    m_IsInitialized = false;
}

std::vector<KeyPressEvent> KeyboardCapture::ConsumeKeyPresses()
{
    std::lock_guard<std::mutex> lock(m_KeyStateMutex);
    auto events = std::move(m_PressQueue);
    m_PressQueue.clear();
    return events;
}

void KeyboardCapture::RequeueKeyPresses(std::vector<KeyPressEvent> events)
{
    if (events.empty())
        return;

    std::lock_guard<std::mutex> lock(m_KeyStateMutex);
    events.insert(events.end(),
                  std::make_move_iterator(m_PressQueue.begin()),
                  std::make_move_iterator(m_PressQueue.end()));
    if (events.size() > MAX_PRESS_QUEUE)
        events.erase(events.begin(), events.begin() + static_cast<std::ptrdiff_t>(events.size() - MAX_PRESS_QUEUE));
    m_PressQueue = std::move(events);
}

std::vector<uint32_t> KeyboardCapture::GetDownKeysSnapshot() const
{
    std::lock_guard<std::mutex> lock(m_KeyStateMutex);
    std::vector<uint32_t> keys;
    keys.reserve(m_KeyDown.size());
    for (const auto &[key, is_down] : m_KeyDown)
    {
        if (is_down)
            keys.push_back(static_cast<uint32_t>(key));
    }
    return keys;
}

UINT KeyboardCapture::NexusWndProcCallback(HWND, UINT uMsg, WPARAM wParam, LPARAM)
{
    const auto virtual_key = static_cast<int>(wParam);
    switch (uMsg)
    {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        GetInstance().ProcessKeyMessage(virtual_key, true);
        break;
    case WM_KEYUP:
    case WM_SYSKEYUP:
        GetInstance().ProcessKeyMessage(virtual_key, false);
        break;
    default:
        break;
    }

    // Nexus assigns the callback return value back to the real message ID.
    return uMsg;
}

void KeyboardCapture::ProcessKeyMessage(int virtual_key, bool is_pressed)
{
    std::lock_guard<std::mutex> lock(m_KeyStateMutex);
    const auto was_pressed = m_KeyDown[virtual_key];

    if (is_pressed && !was_pressed)
    {
        const auto is_down = [this](int key) {
            const auto it = m_KeyDown.find(key);
            return it != m_KeyDown.end() && it->second;
        };

        if (m_PressQueue.size() >= MAX_PRESS_QUEUE)
            m_PressQueue.erase(m_PressQueue.begin());
        m_PressQueue.push_back({
            .virtual_key = static_cast<uint32_t>(virtual_key),
            .shift = is_down(VK_SHIFT) || is_down(VK_LSHIFT) || is_down(VK_RSHIFT) ||
                     virtual_key == VK_SHIFT || virtual_key == VK_LSHIFT || virtual_key == VK_RSHIFT,
            .control = is_down(VK_CONTROL) || is_down(VK_LCONTROL) || is_down(VK_RCONTROL) ||
                       virtual_key == VK_CONTROL || virtual_key == VK_LCONTROL || virtual_key == VK_RCONTROL,
            .alt = is_down(VK_MENU) || is_down(VK_LMENU) || is_down(VK_RMENU) ||
                   virtual_key == VK_MENU || virtual_key == VK_LMENU || virtual_key == VK_RMENU,
        });
    }

    m_KeyDown[virtual_key] = is_pressed;
}
