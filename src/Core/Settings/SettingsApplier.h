// AccessOS/src/Core/Settings/SettingsApplier.h
//
// SettingsApplier — reads SettingsManager and applies values to live subsystems.
//
// Why: SettingsManager knows how to persist settings, but it knows nothing
//      about the runtime subsystems. SettingsApplier is the glue layer that
//      bridges the two without coupling them.
//
// Usage:
//   SettingsApplier::Apply(settings, speechManager, reader, shortcuts);
//
// Threading: Call only during initialization or when subsystems are idle.

#pragma once

#include "SettingsManager.h"
#include "../Speech/SpeechManager.h"
#include "../Reader/AccessReader.h"
#include "../Commands/ShortcutManager.h"
#include "../Commands/CommandManager.h"

namespace AccessOS {

class SettingsApplier {
public:
    /// Apply all persisted settings from manager to the live subsystems.
    /// Any null pointer is skipped safely.
    static void Apply(const SettingsManager& settings,
                      SpeechManager*         speech,
                      AccessReader*          reader,
                      ShortcutManager*       shortcuts,
                      CommandManager*        commands) noexcept;

    /// Persist current live state back to SettingsManager.
    /// Called when the user changes a setting at runtime.
    static void Save(SettingsManager& settings,
                     const SpeechManager* speech) noexcept;

private:
    SettingsApplier() = delete;
};

} // namespace AccessOS
