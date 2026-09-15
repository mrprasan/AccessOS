// AccessOS/src/Core/Commands/KeyboardManager.h
//
// KeyboardManager — low-level keyboard hook and modifier state tracker.
//
// Why: Keyboard input must be captured system-wide via a low-level hook
//      so AccessOS can intercept shortcuts even when focus is in another
//      application. The modifier state is tracked separately because
//      WM_KEYDOWN messages carry only the key code, not full modifier state.
//
// Responsibilities:
//   - Install / remove a WH_KEYBOARD_LL system-wide hook.
//   - Track Shift, Ctrl, Alt, Win modifier state in real time.
//   - Convert raw KBDLLHOOKSTRUCT into a KeyStroke.
//   - Invoke ShortcutManager::Lookup and CommandManager::Execute for each
//     matched keystroke.
//   - Call the raw key callback for unmatched keystrokes so callers can
//     forward them normally.
//
// Threading:
//   - Install/Uninstall must be called from the thread that pumps messages
//     (the hook callback fires on that same thread).
//   - CommandManager::Execute is called on that same thread.
//
// Must NOT:
//   - Speak, log user content, passwords, PINs.
//   - Hold any live COM/UIA pointers.

#pragma once

#include <windows.h>

#include "ShortcutManager.h"
#include "CommandManager.h"

#include <functional>
#include <atomic>

namespace AccessOS {

// Callback invoked for every key event that was NOT consumed by a shortcut.
// Parameters: vkCode, isKeyDown.
using RawKeyCallback = std::function<void(uint32_t vkCode, bool isKeyDown)>;

class KeyboardManager {
public:
    KeyboardManager(ShortcutManager* shortcuts, CommandManager* commands);
    ~KeyboardManager();

    // Install the low-level keyboard hook.
    // Must be called from a thread with a message pump.
    // Returns false if the hook is already installed or SetWindowsHookEx fails.
    bool Install();

    // Remove the low-level keyboard hook.
    void Uninstall();

    // Returns true if the hook is currently installed.
    bool IsInstalled() const noexcept;

    // Register a callback for raw (non-shortcut) key events.
    // Replaces any previous callback.
    void SetRawKeyCallback(RawKeyCallback cb);

    // Query current modifier state.
    bool IsShiftDown() const noexcept;
    bool IsCtrlDown()  const noexcept;
    bool IsAltDown()   const noexcept;
    bool IsWinDown()   const noexcept;

    // Build a KeyStroke from a vkCode using current modifier state.
    KeyStroke MakeKeyStroke(uint32_t vkCode) const noexcept;

private:
    // Low-level keyboard hook procedure (static, forwarded to instance).
    static LRESULT CALLBACK HookProc(int nCode, WPARAM wParam, LPARAM lParam);

    // Per-instance hook processing.
    LRESULT ProcessHook(int nCode, WPARAM wParam, LPARAM lParam);

    // Update modifier flags based on a key event.
    void UpdateModifiers(uint32_t vkCode, bool isDown) noexcept;

    ShortcutManager*  m_shortcuts;
    CommandManager*   m_commands;
    RawKeyCallback    m_rawCallback;
    HHOOK             m_hook      = nullptr;

    // Modifier state — updated in the hook callback.
    std::atomic<bool> m_shift { false };
    std::atomic<bool> m_ctrl  { false };
    std::atomic<bool> m_alt   { false };
    std::atomic<bool> m_win   { false };

    // Pointer to the active instance — required because the Win32 hook
    // callback is a static function. Only one KeyboardManager may be
    // installed at a time.
    static KeyboardManager* s_instance;
};

} // namespace AccessOS
