// AccessOS/src/Core/Announcement/AnnouncementEngine.h
//
// AnnouncementEngine — composes an Announcement from an AccessNode and AppContext.
//
// Why: This is the single place where verbosity rules, context suppression,
//      and part ordering are applied.  No other component decides what to say —
//      they only say it.
//
// Rules (applied in this priority order):
//   R1. If the node is not valid → return empty announcement.
//   R2. Name part is always first (after optional context prefix).
//   R3. Role part follows name, unless verbosity is Minimal or context
//       suppresses it (e.g. Terminal context suppresses "pane", "group").
//   R4. State part follows role (checked, expanded, disabled, …).
//   R5. Value part follows state (sliders, progress bars, edits).
//   R6. Position part follows value when policy.announcePosition is set.
//   R7. Description part is appended only at Detailed verbosity.
//   R8. Context-suppressed roles: in Terminal, roles Window/Pane/Group omitted.
//   R9. Dialog context prepends dialog title as a Context part on first
//       announcement in that dialog (tracked by modal window title).
//
// Threading: Stateless — safe to call from any thread.

#pragma once

#include "Announcement.h"
#include "../Semantic/AccessNode.h"
#include "../Context/AppContext.h"
#include "../Speech/SpeechPolicy.h"

#include <string>

namespace AccessOS {

class AnnouncementEngine {
public:
    AnnouncementEngine() = delete;

    /// Build an Announcement for a focus-change event.
    /// @param node      The newly-focused AccessNode.
    /// @param ctx       Current application context.
    /// @param policy    Speech policy (verbosity, what to include).
    /// @param prevTitle Window title of the previously-focused element.
    ///                  Used to detect dialog title changes (R9).
    static Announcement BuildFocusAnnouncement(
        const AccessNode&  node,
        const AppContext&  ctx,
        const SpeechPolicy& policy,
        const std::string& prevTitle = "") noexcept;

    /// Build an Announcement for a property-change event (value, state).
    static Announcement BuildPropertyAnnouncement(
        const AccessNode&  node,
        const AppContext&  ctx,
        const SpeechPolicy& policy) noexcept;

    /// Build an Announcement for a live-region / alert event.
    static Announcement BuildAlertAnnouncement(
        const AccessNode&  node,
        const AppContext&  ctx) noexcept;

private:
    /// Returns the spoken role string for the given role.
    static const char* RoleText(AccessRole role) noexcept;

    /// Returns state text for the given node under the given policy.
    static std::string StateText(const AccessNode& node,
                                  const SpeechPolicy& policy) noexcept;

    /// Returns value text (suppressed for passwords, empty when not useful).
    static std::string ValueText(const AccessNode& node) noexcept;

    /// Returns position text ("3 of 10") if set data is present.
    static std::string PositionText(const AccessNode& node) noexcept;

    /// Returns true if the role should be suppressed in the given context.
    static bool IsRoleSuppressedInContext(AccessRole role,
                                           AppContextType ctxType) noexcept;
};

} // namespace AccessOS
