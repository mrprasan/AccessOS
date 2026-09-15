// AccessOS/src/Core/Main.cpp
//
// AccessOS native core — entry point for the first milestone.
//
// Milestone goal (Section 50):
//   Start → COM init → UIA init → Get focused element →
//   Convert to AccessNode → Display diagnostic info → Clean shutdown.

#include "Accessibility/UIAutomation/UIAProvider.h"
#include "Logging/Logger.h"
#include "Semantic/AccessNode.h"
#include "Version.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace AccessOS;

namespace {

static constexpr const char* kComponent = "AccessOSMain";

// Maps AccessRole to a human-readable string for diagnostic output.
const char* RoleName(AccessRole role) noexcept {
    switch (role) {
    case AccessRole::Button:        return "Button";
    case AccessRole::CheckBox:      return "CheckBox";
    case AccessRole::ComboBox:      return "ComboBox";
    case AccessRole::Dialog:        return "Dialog";
    case AccessRole::Document:      return "Document";
    case AccessRole::Edit:          return "Edit";
    case AccessRole::Group:         return "Group";
    case AccessRole::Image:         return "Image";
    case AccessRole::Link:          return "Link";
    case AccessRole::ListBox:       return "ListBox";
    case AccessRole::ListItem:      return "ListItem";
    case AccessRole::Menu:          return "Menu";
    case AccessRole::MenuBar:       return "MenuBar";
    case AccessRole::MenuItem:      return "MenuItem";
    case AccessRole::Pane:          return "Pane";
    case AccessRole::ProgressBar:   return "ProgressBar";
    case AccessRole::RadioButton:   return "RadioButton";
    case AccessRole::ScrollBar:     return "ScrollBar";
    case AccessRole::Slider:        return "Slider";
    case AccessRole::Spinner:       return "Spinner";
    case AccessRole::StaticText:    return "StaticText";
    case AccessRole::Tab:           return "Tab";
    case AccessRole::TabControl:    return "TabControl";
    case AccessRole::Table:         return "Table";
    case AccessRole::Tree:          return "Tree";
    case AccessRole::TreeItem:      return "TreeItem";
    case AccessRole::Window:        return "Window";
    case AccessRole::MultiLineEdit: return "MultiLineEdit";
    case AccessRole::ToolBar:       return "ToolBar";
    case AccessRole::Tooltip:       return "Tooltip";
    default:                        return "Unknown";
    }
}

// Prints a structured diagnostic view of an AccessNode to stdout.
void PrintDiagnostic(const AccessNode& node) {
    std::cout << "\n=== AccessOS — Focused Element ===\n";
    std::cout << "  ID          : " << node.id           << "\n";
    std::cout << "  Role        : " << RoleName(node.role) << "\n";
    std::cout << "  Name        : " << (node.name.empty()        ? "(none)" : node.name)        << "\n";
    std::cout << "  Value       : " << (node.value.empty()       ? "(none)" : node.value)       << "\n";
    std::cout << "  Description : " << (node.description.empty() ? "(none)" : node.description) << "\n";
    std::cout << "  HelpText    : " << (node.helpText.empty()    ? "(none)" : node.helpText)    << "\n";
    std::cout << "  ProcessId   : " << node.processId            << "\n";
    std::cout << "  Bounds      : ["
              << node.bounds.left << ", " << node.bounds.top
              << ", " << node.bounds.width << "x" << node.bounds.height << "]\n";

    // State flags
    std::cout << "  State       :";
    if (HasState(node.state, AccessState::Focused))       std::cout << " Focused";
    if (HasState(node.state, AccessState::Focusable))     std::cout << " Focusable";
    if (HasState(node.state, AccessState::Disabled))      std::cout << " Disabled";
    if (HasState(node.state, AccessState::Checked))       std::cout << " Checked";
    if (HasState(node.state, AccessState::Expanded))      std::cout << " Expanded";
    if (HasState(node.state, AccessState::Collapsed))     std::cout << " Collapsed";
    if (HasState(node.state, AccessState::Protected))     std::cout << " Protected";
    if (HasState(node.state, AccessState::Offscreen))     std::cout << " Offscreen";
    if (node.state == AccessState::None)                  std::cout << " (none)";
    std::cout << "\n";

    std::cout << "  Provider    : UIAutomation\n";
    std::cout << "  Valid       : " << (node.isValid ? "yes" : "no") << "\n";
    std::cout << "==================================\n\n";
}

} // anonymous namespace

int main() {
    std::cout << "AccessOS v" ACCESSOS_VERSION_STRING " — Native Core\n";
    std::cout << "Starting...\n\n";

    // ── Step 1: Initialize COM ────────────────────────────────────────────────
    // MTA is required for UI Automation on a background/worker thread.
    // The main thread uses MTA here since this is a console diagnostic tool.
    HRESULT hr = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        std::cerr << "ERROR: COM initialization failed: 0x"
                  << std::hex << hr << "\n";
        return 1;
    }
    std::cout << "[OK] COM initialized (MTA)\n";

    // ── Step 2: Initialize UI Automation provider ─────────────────────────────
    UIAProvider provider;
    auto initResult = provider.Initialize();
    if (initResult.IsError()) {
        std::cerr << "ERROR: UIA initialization failed: "
                  << initResult.Message() << "\n";
        ::CoUninitialize();
        return 1;
    }
    std::cout << "[OK] UI Automation initialized\n";

    // ── Step 3: Poll focused element in a simple loop ─────────────────────────
    std::cout << "\nPolling focused element every 2 seconds.\n";
    std::cout << "Focus a window (e.g. Notepad) and watch the output.\n";
    std::cout << "Press Ctrl+C to exit.\n\n";

    for (int i = 0; i < 10; ++i) {
        auto focusResult = provider.GetFocusedElement();

        if (focusResult.IsError()) {
            std::cout << "[WARN] Could not get focused element: "
                      << focusResult.Message() << "\n";
        } else {
            PrintDiagnostic(focusResult.Value());
        }

        ::Sleep(2000);
    }

    // ── Step 4: Clean shutdown ────────────────────────────────────────────────
    provider.Shutdown();
    std::cout << "[OK] UIA provider shut down\n";

    ::CoUninitialize();
    std::cout << "[OK] COM uninitialized\n";
    std::cout << "AccessOS exiting cleanly.\n";

    return 0;
}
