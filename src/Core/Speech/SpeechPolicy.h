// AccessOS/src/Core/Speech/SpeechPolicy.h
//
// Speech policy — governs verbosity, role announcement, state announcement,
// and interruption rules.
//
// Why: Speech formatting rules must be centralized and configurable.
//      They must NOT be scattered across navigation or event handlers.
//      The policy object is passed to SpeechFormatter at format time.

#pragma once

#include <cstdint>

namespace AccessOS {

// Verbosity levels — controls how much information is announced.
enum class VerbosityLevel : uint8_t {
    Minimal  = 0,   // Name only
    Standard = 1,   // Name + role + key state
    Detailed = 2,   // Name + role + all state + position + description
    Developer = 3,  // Everything including provider info
};

struct SpeechPolicy {
    VerbosityLevel  verbosity           = VerbosityLevel::Standard;

    // What to include in announcements
    bool            announceRole        = true;
    bool            announceState       = true;
    bool            announceValue       = true;
    bool            announceDescription = false;
    bool            announceHelpText    = false;
    bool            announcePosition    = false;   // "3 of 10"
    bool            announceShortcut    = false;

    // Interruption rules
    bool            navigationCancelsNormal  = true;   // Nav cancels Normal speech
    bool            criticalInterruptsAll    = true;   // Critical always interrupts

    // Returns a default standard policy.
    static SpeechPolicy Standard() {
        return SpeechPolicy{};
    }

    // Returns a minimal policy — name only.
    static SpeechPolicy Minimal() {
        SpeechPolicy p;
        p.verbosity        = VerbosityLevel::Minimal;
        p.announceRole     = false;
        p.announceState    = false;
        p.announceValue    = false;
        return p;
    }

    // Returns a detailed policy — maximum information.
    static SpeechPolicy Detailed() {
        SpeechPolicy p;
        p.verbosity              = VerbosityLevel::Detailed;
        p.announceRole           = true;
        p.announceState          = true;
        p.announceValue          = true;
        p.announceDescription    = true;
        p.announceHelpText       = true;
        p.announcePosition       = true;
        return p;
    }
};

} // namespace AccessOS
