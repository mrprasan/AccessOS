// AccessOS/src/Core/Announcement/AnnouncementPart.h
//
// AnnouncementPart — a single speakable segment of an announcement.
//
// Why: Announcements are not monolithic strings.  They are composed of
//      ordered parts (name, role, state, value, position, description)
//      so that callers can insert pauses, change prosody, or suppress
//      individual segments without re-parsing a flat string.
//
// The AnnouncementBuilder assembles parts; SpeechManager renders them.

#pragma once

#include <string>
#include <cstdint>

namespace AccessOS {

/// Identifies what semantic kind of information a part carries.
enum class AnnouncementPartKind : uint8_t {
    Name        = 0,    // Accessible name of the element
    Role        = 1,    // Spoken role string, e.g. "button"
    State       = 2,    // State text, e.g. "checked", "expanded"
    Value       = 3,    // Current value, e.g. "75%" for a slider
    Position    = 4,    // "3 of 10"
    Description = 5,    // Accessible description
    HelpText    = 6,    // Tooltip / help text
    Context     = 7,    // Application/context prefix, e.g. "dialog:"
    Custom      = 8,    // Caller-supplied text that doesn't fit other kinds
};

/// One speakable segment.
struct AnnouncementPart {
    AnnouncementPartKind kind  = AnnouncementPartKind::Custom;
    std::string          text;

    bool IsEmpty() const noexcept { return text.empty(); }
};

} // namespace AccessOS
