// AccessOS/src/Core/Commands/CommandRegistry.h
//
// CommandRegistry — registers all built-in AccessOS commands and
//                   their default keyboard shortcuts.
//
// Why: Commands and shortcuts must be defined in one place so that:
//   - Unit tests can verify registration without a keyboard.
//   - Settings can override any binding without touching command logic.
//   - The DLL bridge can expose a "list commands" export.
//
// Built-in command IDs (string constants):
//   "reader.stop_speech"       — stop all speech
//   "reader.read_focused"      — re-announce the focused element
//   "reader.read_title"        — announce the active window title
//   "nav.next"                 — read next element
//   "nav.prev"                 — read previous element
//   "nav.next_heading"         — jump to next heading
//   "nav.prev_heading"         — jump to previous heading
//   "nav.next_link"            — jump to next link
//   "nav.prev_link"            — jump to previous link
//   "nav.next_button"          — jump to next button
//   "nav.prev_button"          — jump to previous button
//   "nav.next_form_field"      — jump to next form field
//   "nav.prev_form_field"      — jump to previous form field
//   "nav.parent"               — move to parent element
//   "nav.first_child"          — move to first child element

#pragma once

#include "CommandManager.h"
#include "ShortcutManager.h"
#include "../Reader/AccessReader.h"
#include "../Navigation/NavigationEngine.h"
#include "../Speech/SpeechManager.h"
#include "../Focus/FocusManager.h"
#include "../Speech/SpeechFormatter.h"

#include <string>

namespace AccessOS {

// ── Command ID constants ───────────────────────────────────────────────────────

namespace CmdId {
    inline constexpr const char* StopSpeech      = "reader.stop_speech";
    inline constexpr const char* ReadFocused     = "reader.read_focused";
    inline constexpr const char* ReadTitle       = "reader.read_title";
    inline constexpr const char* NavNext         = "nav.next";
    inline constexpr const char* NavPrev         = "nav.prev";
    inline constexpr const char* NavNextHeading  = "nav.next_heading";
    inline constexpr const char* NavPrevHeading  = "nav.prev_heading";
    inline constexpr const char* NavNextLink     = "nav.next_link";
    inline constexpr const char* NavPrevLink     = "nav.prev_link";
    inline constexpr const char* NavNextButton   = "nav.next_button";
    inline constexpr const char* NavPrevButton   = "nav.prev_button";
    inline constexpr const char* NavNextForm     = "nav.next_form_field";
    inline constexpr const char* NavPrevForm     = "nav.prev_form_field";
    inline constexpr const char* NavParent       = "nav.parent";
    inline constexpr const char* NavFirstChild   = "nav.first_child";
}

// ── CommandRegistry ───────────────────────────────────────────────────────────

class CommandRegistry {
public:
    /// Register all built-in commands into manager.
    /// All pointer parameters are non-owning; must outlive the manager.
    static void RegisterAll(CommandManager&    commands,
                            AccessReader*      reader,
                            NavigationEngine*  nav,
                            SpeechManager*     speech,
                            FocusManager*      focus) noexcept;

    /// Bind all default keyboard shortcuts (CapsLock-based) into shortcuts.
    /// CapsLock is represented as VK_CAPITAL (0x14) in the modifier set,
    /// mapped here as a Win-modifier substitute via virtual-key combinations.
    ///
    /// Default bindings:
    ///   CapsLock + Left/Right Arrow → nav.prev / nav.next
    ///   CapsLock + H / Shift+H      → nav.next_heading / nav.prev_heading
    ///   CapsLock + K / Shift+K      → nav.next_link / nav.prev_link
    ///   CapsLock + B / Shift+B      → nav.next_button / nav.prev_button
    ///   CapsLock + E / Shift+E      → nav.next_form_field / nav.prev_form_field
    ///   CapsLock + Up Arrow         → nav.parent
    ///   CapsLock + Down Arrow       → nav.first_child
    ///   CapsLock + Ctrl+S           → reader.stop_speech
    ///   CapsLock + Space            → reader.read_focused
    ///   CapsLock + T                → reader.read_title
    static void BindDefaults(ShortcutManager& shortcuts) noexcept;

private:
    CommandRegistry() = delete;
};

} // namespace AccessOS
