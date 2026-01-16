#pragma once

#include <Windows.h>
#include <functional>
#include <unordered_map>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <chrono>

/**
 * @class KeyboardCapture
 * @brief Modern keyboard input capture using Raw Input API
 *
 * Provides thread-safe, non-blocking keyboard event capture without traditional hooks.
 * Suitable for DLL injection scenarios.
 */
class KeyboardCapture
{
public:
    /**
     * @enum KeyEvent
     * @brief Types of keyboard events
     */
    enum class KeyEvent
    {
        KeyDown,
        KeyUp,
        KeyRepeat  // For held keys with repeating messages
    };

    /**
     * @struct KeyPressInfo
     * @brief Information about a key press event
     */
    struct KeyPressInfo
    {
        USHORT VirtualKey;      // Virtual key code
        KeyEvent Event;         // Type of event
        bool IsExtendedKey;     // True for extended keys (arrows, etc.)
        unsigned long Timestamp; // Time of event in milliseconds
        bool IsInjected;        // True if input was injected/synthetic
    };

    // Type alias for keyboard event callback
    using KeyboardCallback = std::function<void(const KeyPressInfo&)>;

    /**
     * @brief Get singleton instance
     */
    static KeyboardCapture& GetInstance();

    /**
     * @brief Initialize keyboard capture via Nexus WNDPROC hook
     * This method should be called from AddonLoad
     * @param wndprocAddRem Nexus WNDPROC_ADDREM function pointer
     * @return true if initialization succeeded
     */
    bool InitializeNexus(void (*wndprocAddRem)(UINT (*)(HWND, UINT, WPARAM, LPARAM)));

    /**
     * @brief Shutdown and cleanup keyboard capture
     */
    void Shutdown();

    /**
     * @brief Register callback for all key events
     * @param callback Function to call on keyboard event
     */
    void RegisterCallback(KeyboardCallback callback);

    /**
     * @brief Register callback for specific virtual key
     * @param vKey Virtual key code
     * @param callback Function to call when this key is pressed
     */
    void RegisterKeyCallback(USHORT vKey, KeyboardCallback callback);

    /**
     * @brief Check if specific key is currently pressed
     * @param vKey Virtual key code
     * @return true if key is down
     */
    bool IsKeyPressed(USHORT vKey) const;

    /**
     * @brief Get list of all currently pressed keys
     */
    std::vector<USHORT> GetPressedKeys() const;

    /**
     * @brief Process window message (call from WndProc)
     * Must be called from the window's message loop for Raw Input to work
     */
    void ProcessMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    /**
     * @brief Get current state of all keys
     */
    const std::unordered_map<USHORT, bool>& GetKeyStates() const;

    // Prevent copying
    KeyboardCapture(const KeyboardCapture&) = delete;
    KeyboardCapture& operator=(const KeyboardCapture&) = delete;

private:
    KeyboardCapture() = default;
    ~KeyboardCapture();

    static UINT NexusWndProcCallback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void ProcessRawInputMessage(LPARAM lParam);
    void DispatchCallbacks(const KeyPressInfo& info);
    void PollingThreadFunction();

    bool m_IsInitialized = false;
    void (*m_NexusWndProcAddRem)(UINT (*)(HWND, UINT, WPARAM, LPARAM)) = nullptr;

    // Polling thread for key state detection
    std::thread m_PollingThread;
    std::atomic<bool> m_ShouldStopPolling{false};

    // Key state tracking
    mutable std::mutex m_KeyStateMutex;
    std::unordered_map<USHORT, bool> m_KeyStates;

    // Callbacks
    mutable std::mutex m_CallbackMutex;
    std::vector<KeyboardCallback> m_GlobalCallbacks;
    std::unordered_map<USHORT, std::vector<KeyboardCallback>> m_KeySpecificCallbacks;

    // Event queue for async processing if needed
    mutable std::mutex m_EventQueueMutex;
    std::queue<KeyPressInfo> m_EventQueue;
};
