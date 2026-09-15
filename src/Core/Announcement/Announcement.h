// AccessOS/src/Core/Announcement/Announcement.h
//
// Announcement — the complete, ordered set of parts to be spoken for one event.
//
// Why: Having a structured announcement (rather than a flat string) allows:
//   - The speech engine to apply prosody rules per-part.
//   - Tests to verify individual parts without string-parsing.
//   - Future Braille output to select only Name + Role parts.
//
// An Announcement is produced by AnnouncementEngine and consumed by SpeechManager.

#pragma once

#include "AnnouncementPart.h"
#include "../Speech/SpeechPriority.h"

#include <string>
#include <vector>

namespace AccessOS {

struct Announcement {
    /// Ordered parts — spoken left-to-right, joined by comma-space.
    std::vector<AnnouncementPart> parts;

    /// Priority for the speech queue.
    SpeechPriority priority = SpeechPriority::Normal;

    /// True when this announcement should cancel all pending Normal/Low speech.
    bool cancelPrevious = false;

    /// Collapse all parts to a single flat string (for speech engines that
    /// take a plain string).  Parts are joined with ", "; empty parts skipped.
    std::string Flatten() const;

    /// Returns true when there are no non-empty parts.
    bool IsEmpty() const noexcept;

    /// Append a part (ignores empty text).
    void Add(AnnouncementPartKind kind, const std::string& text);

    /// Convenience: add a custom text part.
    void AddText(const std::string& text);
};

} // namespace AccessOS
