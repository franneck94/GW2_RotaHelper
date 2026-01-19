#pragma once

#include <mutex>
#include <unordered_map>
#include <windows.h>

class KeyboardCapture
{
public:
    static KeyboardCapture &GetInstance()
    {
        static KeyboardCapture instance;
        return instance;
    }

    // Initialize with Nexus API callbacks
    bool Initialize(void (*wndprocRegister)(UINT (*)(HWND, UINT, WPARAM, LPARAM)),
                    void (*wndprocDeregister)(UINT (*)(HWND, UINT, WPARAM, LPARAM)));
    void Shutdown();

    // Check key state
    bool IsKeyDown(int vKey) const;
    bool WasKeyPressed(int vKey) const; // True only once per key press

    // Static callback for Nexus WndProc
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
    std::unordered_map<int, bool> m_KeyPressed;
};
