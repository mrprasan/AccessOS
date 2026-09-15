// AccessOS/src/Core/Settings/SettingsApplier.cpp

#include "SettingsApplier.h"
#include "../Speech/SpeechPolicy.h"
#include "../Logging/Logger.h"

namespace AccessOS {

void SettingsApplier::Apply(const SettingsManager& settings,
                             SpeechManager*         speech,
                             AccessReader*          reader,
                             ShortcutManager*       shortcuts,
                             CommandManager*        commands) noexcept
{
    // ── Speech engine settings ────────────────────────────────────────────────
    if (speech) {
        speech->SetRate(settings.SpeechRate());
        speech->SetVolume(settings.SpeechVolume());
        const std::string voice = settings.VoiceId();
        if (!voice.empty()) speech->SetVoice(voice);
    }

    // ── Verbosity / policy ────────────────────────────────────────────────────
    if (reader) {
        SpeechPolicy pol = reader->GetPolicy();
        const int verbosity = settings.Verbosity();
        switch (verbosity) {
        case 0:
            pol.verbosity           = VerbosityLevel::Minimal;
            pol.announceRole        = false;
            pol.announceState       = false;
            pol.announceValue       = false;
            pol.announceDescription = false;
            pol.announcePosition    = false;
            break;
        case 2:
            pol.verbosity           = VerbosityLevel::Detailed;
            pol.announceDescription = true;
            pol.announcePosition    = settings.AnnouncePosition();
            break;
        default:  // 1 = Standard
            pol.verbosity           = VerbosityLevel::Standard;
            pol.announceDescription = false;
            pol.announcePosition    = settings.AnnouncePosition();
            break;
        }
        reader->SetPolicy(pol);
        if (speech) speech->SetPolicy(pol);
    }

    // ── Shortcut bindings from persisted settings ─────────────────────────────
    // Custom bindings are stored as "shortcuts.<commandId>" in the DB.
    // They override the defaults already bound. We iterate all registered
    // commands and check whether a custom binding was saved.
    if (shortcuts && commands) {
        for (const auto& id : commands->AllIds()) {
            auto [vk, mods] = settings.GetShortcut(id);
            if (vk != 0) {
                KeyStroke ks{ vk, static_cast<KeyModifier>(mods) };
                // Rebind if already bound, bind fresh if not.
                if (shortcuts->HasConflict(ks)) {
                    shortcuts->Rebind(ks, id);
                } else {
                    shortcuts->Bind(ks, id);
                }
            }
        }
    }

    ACOS_LOG_INFO("SettingsApplier", "Settings applied to runtime subsystems");
}

void SettingsApplier::Save(SettingsManager&     settings,
                            const SpeechManager* speech) noexcept
{
    if (!speech) return;

    SpeechPolicy pol = speech->GetPolicy();
    settings.SetSpeechRate(speech->GetPolicy().verbosity == VerbosityLevel::Minimal
                           ? settings.SpeechRate()  // don't clobber — rate unchanged
                           : settings.SpeechRate());
    // Persist verbosity level.
    int level = 1;
    switch (pol.verbosity) {
    case VerbosityLevel::Minimal:  level = 0; break;
    case VerbosityLevel::Detailed: level = 2; break;
    case VerbosityLevel::Developer:level = 3; break;
    default:                        level = 1; break;
    }
    settings.SetVerbosity(level);
    settings.SetAnnouncePosition(pol.announcePosition);
}

} // namespace AccessOS
