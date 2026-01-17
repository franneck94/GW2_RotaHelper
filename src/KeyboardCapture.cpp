#include "KeyboardCapture.h"
#include <cassert>

KeyboardCapture& KeyboardCapture::GetInstance()
{
    static KeyboardCapture instance;
    return instance;
}

KeyboardCapture::~KeyboardCapture()
{
    Shutdown();
}

bool KeyboardCapture::InitializeNexus(void (*wndprocAddRem)(UINT (*)(HWND, UINT, WPARAM, LPARAM)))
{
    if (m_IsInitialized)
    {
        return true;
    }

    assert(wndprocAddRem != nullptr && "WNDPROC_ADDREM callback cannot be null");

    m_NexusWndProcAddRem = wndprocAddRem;

    // Start polling thread for key state changes
    m_ShouldStopPolling = false;
    m_PollingThread = std::thread(&KeyboardCapture::PollingThreadFunction, this);

    m_IsInitialized = true;
    return true;
}

UINT KeyboardCapture::NexusWndProcCallback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Only process WM_INPUT messages for keyboard input
    if (uMsg == WM_INPUT)
    {
        // Get the raw input to check if it's keyboard input
        UINT dwSize = 0;
        ::GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, nullptr, &dwSize, sizeof(RAWINPUTHEADER));

        if (dwSize > 0)
        {
            std::vector<BYTE> lpb(dwSize);
            if (::GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, lpb.data(), &dwSize, sizeof(RAWINPUTHEADER)) == dwSize)
            {
                RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(lpb.data());

                // Only process keyboard input, let mouse input pass through
                if (raw->header.dwType == RIM_TYPEKEYBOARD)
                {
                    KeyboardCapture::GetInstance().ProcessMessage(uMsg, wParam, lParam);
                }
            }
        }
    }

    // Return 0 to let the original window procedure handle the message
    // This ensures mouse clicks and other input still work normally
    return 0;
}

void KeyboardCapture::Shutdown()
{
    if (!m_IsInitialized)
    {
        return;
    }

    // Stop polling thread
    m_ShouldStopPolling = true;
    if (m_PollingThread.joinable())
    {
        m_PollingThread.join();
    }

    {
        std::lock_guard<std::mutex> lock(m_KeyStateMutex);
        m_KeyStates.clear();
    }

    {
        std::lock_guard<std::mutex> lock(m_CallbackMutex);
        m_GlobalCallbacks.clear();
        m_KeySpecificCallbacks.clear();
    }

    {
        std::lock_guard<std::mutex> lock(m_EventQueueMutex);
        while (!m_EventQueue.empty())
        {
            m_EventQueue.pop();
        }
    }

    m_IsInitialized = false;
    m_NexusWndProcAddRem = nullptr;
}

void KeyboardCapture::RegisterCallback(KeyboardCallback callback)
{
    std::lock_guard<std::mutex> lock(m_CallbackMutex);
    m_GlobalCallbacks.push_back(callback);
}

void KeyboardCapture::RegisterKeyCallback(USHORT vKey, KeyboardCallback callback)
{
    std::lock_guard<std::mutex> lock(m_CallbackMutex);
    m_KeySpecificCallbacks[vKey].push_back(callback);
}

bool KeyboardCapture::IsKeyPressed(USHORT vKey) const
{
    // Use GetAsyncKeyState for non-intrusive key detection
    // This doesn't interfere with game input processing
    SHORT keyState = ::GetAsyncKeyState(vKey);
    bool isPressed = (keyState & 0x8000) != 0;

    // Update our internal state for consistency
    {
        std::lock_guard<std::mutex> lock(m_KeyStateMutex);
        const_cast<KeyboardCapture*>(this)->m_KeyStates[vKey] = isPressed;
    }

    return isPressed;
}

std::vector<USHORT> KeyboardCapture::GetPressedKeys() const
{
    std::vector<USHORT> pressed;

    // Poll all common virtual key codes
    for (USHORT vKey = 1; vKey < 256; ++vKey)
    {
        if (IsKeyPressed(vKey))
        {
            pressed.push_back(vKey);
        }
    }

    return pressed;
}

const std::unordered_map<USHORT, bool>& KeyboardCapture::GetKeyStates() const
{
    return m_KeyStates;
}

void KeyboardCapture::ProcessMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_INPUT)
    {
        ProcessRawInputMessage(lParam);
    }
}

void KeyboardCapture::ProcessRawInputMessage(LPARAM lParam)
{
    UINT dwSize = 0;
    ::GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, nullptr, &dwSize, sizeof(RAWINPUTHEADER));

    if (dwSize == 0)
    {
        return;
    }

    std::vector<BYTE> lpb(dwSize);
    if (::GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, lpb.data(), &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
    {
        return;
    }

    RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(lpb.data());

    if (raw->header.dwType != RIM_TYPEKEYBOARD)
    {
        return;
    }

    const RAWKEYBOARD& kb = raw->data.keyboard;

    // Skip if it's not a valid key event
    if (kb.VKey == 0 || kb.VKey >= 256)
    {
        return;
    }

    // Determine key event type
    KeyEvent eventType = KeyEvent::KeyDown;
    if (kb.Flags & RI_KEY_BREAK)
    {
        eventType = KeyEvent::KeyUp;
    }
    else if (kb.Flags & RI_KEY_MAKE)
    {
        eventType = KeyEvent::KeyDown;
    }

    KeyPressInfo info{};
    info.VirtualKey = kb.VKey;
    info.Event = eventType;
    info.IsExtendedKey = (kb.Flags & RI_KEY_E0) != 0 || (kb.Flags & RI_KEY_E1) != 0;
    info.Timestamp = ::GetTickCount();
    info.IsInjected = (kb.Flags & RI_KEY_TERMSRV_SET_LED) != 0;

    // Quick state update with minimal lock time
    {
        std::lock_guard<std::mutex> lock(m_KeyStateMutex);
        m_KeyStates[kb.VKey] = (eventType == KeyEvent::KeyDown);
    }

    // Skip callback dispatch if no callbacks registered to avoid unnecessary work
    {
        std::lock_guard<std::mutex> lock(m_CallbackMutex);
        if (m_GlobalCallbacks.empty() &&
            m_KeySpecificCallbacks.find(info.VirtualKey) == m_KeySpecificCallbacks.end())
        {
            return;
        }
    }

    // Dispatch callbacks (this is the potentially expensive part)
    DispatchCallbacks(info);
}

void KeyboardCapture::DispatchCallbacks(const KeyPressInfo& info)
{
    std::lock_guard<std::mutex> lock(m_CallbackMutex);

    // Call global callbacks
    for (auto& callback : m_GlobalCallbacks)
    {
        callback(info);
    }

    // Call key-specific callbacks
    auto it = m_KeySpecificCallbacks.find(info.VirtualKey);
    if (it != m_KeySpecificCallbacks.end())
    {
        for (auto& callback : it->second)
        {
            callback(info);
        }
    }
}

void KeyboardCapture::PollingThreadFunction()
{
    std::unordered_map<USHORT, bool> previousKeyStates;

    while (!m_ShouldStopPolling)
    {
        for (USHORT vKey = 1; vKey < 256; ++vKey)
        {
            if (vKey >= VK_LBUTTON && vKey <= VK_XBUTTON2)
                continue;

            SHORT keyState = ::GetAsyncKeyState(vKey);
            bool isCurrentlyPressed = (keyState & 0x8000) != 0;

            auto prevIt = previousKeyStates.find(vKey);
            bool wasPreviouslyPressed = (prevIt != previousKeyStates.end()) ? prevIt->second : false;

            if (isCurrentlyPressed != wasPreviouslyPressed)
            {
                {
                    std::lock_guard<std::mutex> lock(m_KeyStateMutex);
                    m_KeyStates[vKey] = isCurrentlyPressed;
                }

                // Create event info
                KeyPressInfo info{};
                info.VirtualKey = vKey;
                info.Event = isCurrentlyPressed ? KeyEvent::KeyDown : KeyEvent::KeyUp;
                info.IsExtendedKey = (vKey >= VK_PRIOR && vKey <= VK_DOWN) || // Page Up/Down, End, Home, Arrow keys
                                   (vKey >= VK_INSERT && vKey <= VK_DELETE) ||  // Insert, Delete
                                   (vKey >= VK_DIVIDE && vKey <= VK_RMENU);     // Numpad Divide, Right Alt
                info.Timestamp = ::GetTickCount();
                info.IsInjected = false; // Can't detect injection with GetAsyncKeyState

                bool hasListeners = false;
                {
                    std::lock_guard<std::mutex> lock(m_CallbackMutex);
                    hasListeners = !m_GlobalCallbacks.empty() ||
                                 m_KeySpecificCallbacks.find(vKey) != m_KeySpecificCallbacks.end();
                }

                if (hasListeners)
                    DispatchCallbacks(info);

                previousKeyStates[vKey] = isCurrentlyPressed;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
