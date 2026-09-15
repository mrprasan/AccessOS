// AccessOS/src/Core/Speech/SpeechPriority.h
//
// Speech priority levels.
//
// Why: Different speech events have different urgency.
//      A critical alert must interrupt normal reading.
//      Navigation cancels stale announcements.
//      These rules must be centralized, not scattered.

#pragma once

#include <cstdint>

namespace AccessOS {

enum class SpeechPriority : uint8_t {
    // Low: informational background messages. Queued behind everything else.
    Low = 0,

    // Normal: standard navigation and property announcements.
    Normal = 1,

    // High: important notifications. Interrupts Normal and Low.
    High = 2,

    // Critical: errors, alerts, system messages.
    // Interrupts all queued speech immediately.
    Critical = 3,
};

} // namespace AccessOS
