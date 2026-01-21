#include <algorithm>

#include "KeyboardCapture.h"
#include "Shared.h"


bool KeyboardCapture::Initialize(void (*wndprocRegister)(UINT (*)(HWND, UINT, WPARAM, LPARAM)),
                                 void (*wndprocDeregister)(UINT (*)(HWND, UINT, WPARAM, LPARAM)))
{
    if (m_IsInitialized)
        return true;

    if (!wndprocRegister || !wndprocDeregister)
        return false;

    // Store deregister function for cleanup
    m_WndProcDeregister = wndprocDeregister;

    m_IsInitialized = true;

    // Register our WndProc callback with Nexus
    wndprocRegister(NexusWndProcCallback);

    return true;
}

void KeyboardCapture::Shutdown()
{
    if (!m_IsInitialized)
        return;

    // Deregister from Nexus
    if (m_WndProcDeregister)
    {
        m_WndProcDeregister(NexusWndProcCallback);
    }

    {
        std::lock_guard<std::mutex> lock(m_KeyStateMutex);
        m_KeyDown.clear();
        m_KeyPressed.clear();
        Globals::CurrentlyPressedKeys.clear();
    }

    m_WndProcDeregister = nullptr;
    m_IsInitialized = false;
}

bool KeyboardCapture::IsKeyDown(int vKey) const
{
    std::lock_guard<std::mutex> lock(m_KeyStateMutex);
    auto it = m_KeyDown.find(vKey);
    return it != m_KeyDown.end() && it->second;
}

bool KeyboardCapture::WasKeyPressed(int vKey) const
{
    std::lock_guard<std::mutex> lock(m_KeyStateMutex);
    auto it = m_KeyPressed.find(vKey);
    if (it != m_KeyPressed.end() && it->second)
    {
        // Clear the flag after reading
        const_cast<KeyboardCapture *>(this)->m_KeyPressed[vKey] = false;
        return true;
    }
    return false;
}

UINT KeyboardCapture::NexusWndProcCallback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    int vKey = static_cast<int>(wParam);

    switch (uMsg)
    {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        GetInstance().ProcessKeyMessage(vKey, true);
        break;

    case WM_KEYUP:
    case WM_SYSKEYUP:
        GetInstance().ProcessKeyMessage(vKey, false);
        break;

    default:
        break;
    }

    return 1;
}

void KeyboardCapture::ProcessKeyMessage(int vKey, bool isPressed)
{
    std::lock_guard<std::mutex> lock(m_KeyStateMutex);

    bool wasPressed = m_KeyDown[vKey];
    m_KeyDown[vKey] = isPressed;

    if (isPressed && !wasPressed)
        m_KeyPressed[vKey] = true;

    auto& currentKeys = Globals::CurrentlyPressedKeys;
    auto it = std::find(currentKeys.begin(), currentKeys.end(), static_cast<uint32_t>(vKey));

    if (isPressed)
    {
        // Key is pressed - add it if it's not already in the list
        if (it == currentKeys.end())
            currentKeys.push_back(static_cast<uint32_t>(vKey));
    }
    else
    {
        // Key is released - remove it if it's in the list
        if (it != currentKeys.end())
            currentKeys.erase(it);

        // Also ensure the key state is properly reset
        m_KeyDown[vKey] = false;
        m_KeyPressed[vKey] = false;
    }
}
