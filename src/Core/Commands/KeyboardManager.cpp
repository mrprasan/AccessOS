// AccessOS/src/Core/Commands/KeyboardManager.cpp

#include "KeyboardManager.h"
#include "../Logging/Logger.h"

namespace AccessOS {

static constexpr const char* kComponent = "KeyboardManager";

// ── Static instance pointer ───────────────────────────────────────────────────
KeyboardManager* KeyboardManager::s_instance = nullptr;

// ── Constructor / Destructor ──────────────────────────────────────────────────

KeyboardManager::KeyboardManager(ShortcutManager* shortcuts,
                                 CommandManager*  commands)
    : m_shortcuts(shortcuts)
    , m_commands(commands)
{}

KeyboardManager::~KeyboardManager() {
    Uninstall();
}

// ── Install / Uninstall ───────────────────────────────────────────────────────

bool KeyboardManager::Install() {
    if (m_hook) return false;  // Already installed.

    s_instance = this;
    m_hook = SetWindowsHookExW(WH_KEYBOARD_LL, HookProc, nullptr, 0);
    if (!m_hook) {
        s_instance = nullptr;
        ACOS_LOG_ERROR(kComponent, "SetWindowsHookEx failed");
        return false;
    }
    ACOS_LOG_INFO(kComponent, "Low-level keyboard hook installed");
    return true;
}

void KeyboardManager::Uninstall() {
    if (!m_hook) return;
    UnhookWindowsHookEx(m_hook);
    m_hook     = nullptr;
    s_instance = nullptr;
    ACOS_LOG_INFO(kComponent, "Low-level keyboard hook removed");
}

bool KeyboardManager::IsInstalled() const noexcept {
    return m_hook != nullptr;
}

// ── Callback registration ─────────────────────────────────────────────────────

void KeyboardManager::SetRawKeyCallback(RawKeyCallback cb) {
    m_rawCallback = std::move(cb);
}

// ── Modifier queries ──────────────────────────────────────────────────────────

bool KeyboardManager::IsShiftDown() const noexcept { return m_shift.load(); }
bool KeyboardManager::IsCtrlDown()  const noexcept { return m_ctrl.load();  }
bool KeyboardManager::IsAltDown()   const noexcept { return m_alt.load();   }
bool KeyboardManager::IsWinDown()   const noexcept { return m_win.load();   }

KeyStroke KeyboardManager::MakeKeyStroke(uint32_t vkCode) const noexcept {
    KeyModifier mods = KeyModifier::None;
    if (m_shift.load()) mods = mods | KeyModifier::Shift;
    if (m_ctrl.load())  mods = mods | KeyModifier::Ctrl;
    if (m_alt.load())   mods = mods | KeyModifier::Alt;
    if (m_win.load())   mods = mods | KeyModifier::Win;
    return KeyStroke{ vkCode, mods };
}

// ── Private helpers ───────────────────────────────────────────────────────────

void KeyboardManager::UpdateModifiers(uint32_t vkCode, bool isDown) noexcept {
    switch (vkCode) {
        case VK_SHIFT:   case VK_LSHIFT:   case VK_RSHIFT:
            m_shift.store(isDown); break;
        case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL:
            m_ctrl.store(isDown);  break;
        case VK_MENU:    case VK_LMENU:    case VK_RMENU:
            m_alt.store(isDown);   break;
        case VK_LWIN:    case VK_RWIN:
            m_win.store(isDown);   break;
        default: break;
    }
}

// ── Hook procedure ────────────────────────────────────────────────────────────

// Static entry point — delegates to the live instance.
LRESULT CALLBACK KeyboardManager::HookProc(int nCode, WPARAM wParam,
                                            LPARAM lParam)
{
    if (s_instance) {
        return s_instance->ProcessHook(nCode, wParam, lParam);
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

LRESULT KeyboardManager::ProcessHook(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode < 0) {
        return CallNextHookEx(m_hook, nCode, wParam, lParam);
    }

    const auto* kbd = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam);
    const uint32_t vkCode  = kbd->vkCode;
    const bool     isDown  = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);

    UpdateModifiers(vkCode, isDown);

    // Only attempt shortcut dispatch on key-down.
    if (isDown && m_shortcuts && m_commands) {
        const KeyStroke ks = MakeKeyStroke(vkCode);
        auto commandId = m_shortcuts->Lookup(ks);
        if (commandId.has_value()) {
            m_commands->Execute(*commandId);
            // Consume the keystroke — do not pass to the next hook.
            return 1;
        }
    }

    // Not a shortcut — fire the raw callback and pass through.
    if (m_rawCallback) {
        m_rawCallback(vkCode, isDown);
    }

    return CallNextHookEx(m_hook, nCode, wParam, lParam);
}

} // namespace AccessOS
