// AccessOS/src/Core/Context/ContextDetector.h
//
// ContextDetector — classifies an application into an AppContextType.
//
// Why: Context detection must be centralised so that all consumers
//      (speech, navigation, Braille) see the same classification.
//      Detection is purely data-driven — process name, window class,
//      window title patterns.
//
// Threading: Stateless — safe to call from any thread.

#pragma once

#include "AppContext.h"
#include "../Semantic/AccessNode.h"
#include <string_view>

namespace AccessOS {

class ContextDetector {
public:
    /// Build an AppContext from a focused AccessNode.
    /// Uses processName, windowTitle, applicationName, and role.
    static AppContext Detect(const AccessNode& node) noexcept;

    /// Classify an app purely by process name (lowercase, with extension).
    static AppContextType ClassifyByProcess(std::string_view procName) noexcept;

    /// Classify by Win32 window class name.
    static AppContextType ClassifyByWindowClass(std::string_view cls) noexcept;

    /// Refine context for a browser: detect address bar vs content area.
    static AppContextType RefineBrowserContext(const AccessNode& node) noexcept;

    /// Returns a human-readable display name for a process name.
    static std::string DisplayName(std::string_view procName) noexcept;

private:
    ContextDetector() = delete;
};

} // namespace AccessOS
