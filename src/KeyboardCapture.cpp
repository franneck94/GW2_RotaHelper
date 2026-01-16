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

bool KeyboardCapture::Initialize(HWND hwnd)
{
    if (m_IsInitialized)
    {
        return true;
    }

    assert(hwnd != nullptr && "Window handle cannot be null");

    m_TargetWindow = hwnd;

    if (!RegisterRawInputDevice(hwnd))
    {
        return false;
    }

    m_IsInitialized = true;
    return true;
}

void KeyboardCapture::Shutdown()
{
    if (!m_IsInitialized)
    {
        return;
    }

    // Unregister raw input device
    RAWINPUTDEVICE rid = {};
    rid.usUsagePage = 0x01;  // Generic desktop
    rid.usUsage = 0x06;      // Keyboard
    rid.dwFlags = RIDEV_REMOVE;
    rid.hwndTarget = nullptr;

    if (!RegisterRawInputDevice(&rid, 1, sizeof(rid)))
    {
        // Log warning but continue cleanup
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
    m_TargetWindow = nullptr;
}

bool KeyboardCapture::RegisterRawInputDevice(HWND hwnd)
{
    RAWINPUTDEVICE rid = {};
    rid.usUsagePage = 0x01;  // Generic desktop control
    rid.usUsage = 0x06;      // Keyboard
    rid.dwFlags = RIDEV_INPUTSINK;  // Receive input even when not in focus
    rid.hwndTarget = hwnd;

    if (!::RegisterRawInputDevice(&rid, 1, sizeof(rid)))
    {
        return false;
    }

    return true;
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
    std::lock_guard<std::mutex> lock(m_KeyStateMutex);
    auto it = m_KeyStates.find(vKey);
    return it != m_KeyStates.end() && it->second;
}

std::vector<USHORT> KeyboardCapture::GetPressedKeys() const
{
    std::lock_guard<std::mutex> lock(m_KeyStateMutex);
    std::vector<USHORT> pressed;
    for (const auto& [key, isPressed] : m_KeyStates)
    {
        if (isPressed)
        {
            pressed.push_back(key);
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

    // Update key state
    {
        std::lock_guard<std::mutex> lock(m_KeyStateMutex);
        m_KeyStates[kb.VKey] = (eventType == KeyEvent::KeyDown);
    }

    // Queue event
    {
        std::lock_guard<std::mutex> lock(m_EventQueueMutex);
        m_EventQueue.push(info);
    }

    // Dispatch callbacks immediately
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
