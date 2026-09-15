// AccessOS/src/Core/Announcement/Announcement.cpp

#include "Announcement.h"

namespace AccessOS {

std::string Announcement::Flatten() const {
    std::string result;
    for (const auto& p : parts) {
        if (p.IsEmpty()) continue;
        if (!result.empty()) result += ", ";
        result += p.text;
    }
    return result;
}

bool Announcement::IsEmpty() const noexcept {
    for (const auto& p : parts) {
        if (!p.IsEmpty()) return false;
    }
    return true;
}

void Announcement::Add(AnnouncementPartKind kind, const std::string& text) {
    if (text.empty()) return;
    parts.push_back({ kind, text });
}

void Announcement::AddText(const std::string& text) {
    Add(AnnouncementPartKind::Custom, text);
}

} // namespace AccessOS
