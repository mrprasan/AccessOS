// AccessOS/src/Core/Commands/CommandRegistry.cpp

#include "CommandRegistry.h"
#include "../Logging/Logger.h"
#include "../Diagnostics/DiagnosticsManager.h"
#include "../Speech/SpeechPolicy.h"

#include <windows.h>   // VK_* constants for BindDefaults

namespace AccessOS {

// ── RegisterAll ───────────────────────────────────────────────────────────────

void CommandRegistry::RegisterAll(CommandManager&   commands,
                                   AccessReader*     reader,
                                   NavigationEngine* nav,
                                   SpeechManager*    speech,
                                   FocusManager*     focus) noexcept
{
    // ── Speech control ────────────────────────────────────────────────────────

    commands.Register(CmdId::StopSpeech, "Stop Speech", [speech]() {
        if (speech) speech->Stop();
        DiagnosticsManager::RecordSpeechCancel();
    });

    commands.Register(CmdId::ReadFocused, "Read Focused Element", [reader]() {
        if (reader) reader->ReadFocused();
    });

    commands.Register(CmdId::ReadTitle, "Read Window Title", [focus, speech]() {
        if (!focus || !speech) return;
        auto node = focus->GetCurrentFocus();
        if (!node.has_value()) return;
        const std::string& title = node->windowTitle;
        if (!title.empty()) {
            speech->SpeakText(title, SpeechPriority::High, true);
        }
    });

    // ── Navigation ────────────────────────────────────────────────────────────

    commands.Register(CmdId::NavNext, "Read Next Element", [nav, speech]() {
        if (!nav || !speech) return;
        auto node = nav->Navigate(NavigationDirection::Next);
        if (node.has_value()) {
            speech->SpeakText(
                SpeechFormatter::Format(node.value(), SpeechPolicy::Standard()),
                SpeechPriority::Normal, true);
            DiagnosticsManager::RecordFocusChange();
        }
    });

    commands.Register(CmdId::NavPrev, "Read Previous Element", [nav, speech]() {
        if (!nav || !speech) return;
        auto node = nav->Navigate(NavigationDirection::Previous);
        if (node.has_value()) {
            speech->SpeakText(
                SpeechFormatter::Format(node.value(), SpeechPolicy::Standard()),
                SpeechPriority::Normal, true);
            DiagnosticsManager::RecordFocusChange();
        }
    });

    commands.Register(CmdId::NavNextHeading, "Next Heading", [nav, speech]() {
        if (!nav || !speech) return;
        auto node = nav->NextHeading();
        if (node.has_value())
            speech->SpeakText(
                SpeechFormatter::Format(node.value(), SpeechPolicy::Standard()),
                SpeechPriority::Normal, true);
    });

    commands.Register(CmdId::NavPrevHeading, "Previous Heading", [nav, speech]() {
        if (!nav || !speech) return;
        auto node = nav->PrevHeading();
        if (node.has_value())
            speech->SpeakText(
                SpeechFormatter::Format(node.value(), SpeechPolicy::Standard()),
                SpeechPriority::Normal, true);
    });

    commands.Register(CmdId::NavNextLink, "Next Link", [nav, speech]() {
        if (!nav || !speech) return;
        auto node = nav->NextLink();
        if (node.has_value())
            speech->SpeakText(
                SpeechFormatter::Format(node.value(), SpeechPolicy::Standard()),
                SpeechPriority::Normal, true);
    });

    commands.Register(CmdId::NavPrevLink, "Previous Link", [nav, speech]() {
        if (!nav || !speech) return;
        auto node = nav->PrevLink();
        if (node.has_value())
            speech->SpeakText(
                SpeechFormatter::Format(node.value(), SpeechPolicy::Standard()),
                SpeechPriority::Normal, true);
    });

    commands.Register(CmdId::NavNextButton, "Next Button", [nav, speech]() {
        if (!nav || !speech) return;
        auto node = nav->NextButton();
        if (node.has_value())
            speech->SpeakText(
                SpeechFormatter::Format(node.value(), SpeechPolicy::Standard()),
                SpeechPriority::Normal, true);
    });

    commands.Register(CmdId::NavPrevButton, "Previous Button", [nav, speech]() {
        if (!nav || !speech) return;
        auto node = nav->PrevButton();
        if (node.has_value())
            speech->SpeakText(
                SpeechFormatter::Format(node.value(), SpeechPolicy::Standard()),
                SpeechPriority::Normal, true);
    });

    commands.Register(CmdId::NavNextForm, "Next Form Field", [nav, speech]() {
        if (!nav || !speech) return;
        auto node = nav->NextFormField();
        if (node.has_value())
            speech->SpeakText(
                SpeechFormatter::Format(node.value(), SpeechPolicy::Standard()),
                SpeechPriority::Normal, true);
    });

    commands.Register(CmdId::NavPrevForm, "Previous Form Field", [nav, speech]() {
        if (!nav || !speech) return;
        auto node = nav->PrevFormField();
        if (node.has_value())
            speech->SpeakText(
                SpeechFormatter::Format(node.value(), SpeechPolicy::Standard()),
                SpeechPriority::Normal, true);
    });

    commands.Register(CmdId::NavParent, "Move to Parent Element", [nav, speech]() {
        if (!nav || !speech) return;
        auto node = nav->GetParent();
        if (node.has_value())
            speech->SpeakText(
                SpeechFormatter::Format(node.value(), SpeechPolicy::Standard()),
                SpeechPriority::Normal, true);
    });

    commands.Register(CmdId::NavFirstChild, "Move to First Child", [nav, speech]() {
        if (!nav || !speech) return;
        auto node = nav->GetFirstChild();
        if (node.has_value())
            speech->SpeakText(
                SpeechFormatter::Format(node.value(), SpeechPolicy::Standard()),
                SpeechPriority::Normal, true);
    });

    ACOS_LOG_INFO("CommandRegistry", "All built-in commands registered");
}

// ── BindDefaults ──────────────────────────────────────────────────────────────
//
// CapsLock is not a Win32 modifier in KBDLLHOOKSTRUCT — it is a regular key
// (VK_CAPITAL = 0x14).  KeyboardManager tracks it as a "Win" modifier because
// CapsLock is used as the AccessOS modifier key.
// So "CapsLock + H" → { VK_H, KeyModifier::Win }.

void CommandRegistry::BindDefaults(ShortcutManager& shortcuts) noexcept {
    using M = KeyModifier;

    const auto W    = M::Win;               // CapsLock acts as Win modifier
    const auto WS   = M::Win | M::Shift;    // CapsLock + Shift
    const auto WC   = M::Win | M::Ctrl;     // CapsLock + Ctrl

    // Navigation
    shortcuts.Bind({ VK_RIGHT, W  }, CmdId::NavNext);
    shortcuts.Bind({ VK_LEFT,  W  }, CmdId::NavPrev);
    shortcuts.Bind({ 'H',      W  }, CmdId::NavNextHeading);
    shortcuts.Bind({ 'H',      WS }, CmdId::NavPrevHeading);
    shortcuts.Bind({ 'K',      W  }, CmdId::NavNextLink);
    shortcuts.Bind({ 'K',      WS }, CmdId::NavPrevLink);
    shortcuts.Bind({ 'B',      W  }, CmdId::NavNextButton);
    shortcuts.Bind({ 'B',      WS }, CmdId::NavPrevButton);
    shortcuts.Bind({ 'E',      W  }, CmdId::NavNextForm);
    shortcuts.Bind({ 'E',      WS }, CmdId::NavPrevForm);
    shortcuts.Bind({ VK_UP,    W  }, CmdId::NavParent);
    shortcuts.Bind({ VK_DOWN,  W  }, CmdId::NavFirstChild);

    // Reader control
    shortcuts.Bind({ 'S',      WC }, CmdId::StopSpeech);
    shortcuts.Bind({ VK_SPACE, W  }, CmdId::ReadFocused);
    shortcuts.Bind({ 'T',      W  }, CmdId::ReadTitle);

    ACOS_LOG_INFO("CommandRegistry", "Default shortcuts bound");
}

} // namespace AccessOS
