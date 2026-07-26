#pragma once

#include <mutex>
#include <unordered_map>
#include <vector>
#include <windows.h>

struct KeyPressEvent
{
    uint32_t virtual_key = 0;
    bool shift = false;
    bool control = false;
    bool alt = false;
};

class KeyboardCapture
{
public:
    static KeyboardCapture &GetInstance()
    {
        static KeyboardCapture instance;
        return instance;
    }

    bool Initialize(void (*wndprocRegister)(UINT (*)(HWND, UINT, WPARAM, LPARAM)),
                    void (*wndprocDeregister)(UINT (*)(HWND, UINT, WPARAM, LPARAM)));
    void Shutdown();

    std::vector<KeyPressEvent> ConsumeKeyPresses();
    void RequeueKeyPresses(std::vector<KeyPressEvent> events);
    std::vector<uint32_t> GetDownKeysSnapshot() const;
    bool IsInitialized() const noexcept
    {
        return m_IsInitialized;
    }

    static UINT NexusWndProcCallback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    KeyboardCapture() = default;
    ~KeyboardCapture()
    {
        Shutdown();
    }
    KeyboardCapture(const KeyboardCapture &) = delete;
    KeyboardCapture &operator=(const KeyboardCapture &) = delete;

    void ProcessKeyMessage(int vKey, bool isPressed);

    bool m_IsInitialized = false;
    void (*m_WndProcDeregister)(UINT (*)(HWND, UINT, WPARAM, LPARAM)) = nullptr;
    mutable std::mutex m_KeyStateMutex;
    std::unordered_map<int, bool> m_KeyDown;
    std::vector<KeyPressEvent> m_PressQueue;
};
