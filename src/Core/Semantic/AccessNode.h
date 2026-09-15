// AccessOS/src/Core/Semantic/AccessNode.h
//
// The AccessOS semantic model — the central representation of an accessible element.
//
// Why: The architecture requires a provider-independent semantic layer.
//      Raw UIA/MSAA/IA2 objects must never flow beyond their acquisition layer.
//      All navigation, speech, Braille, and inspection work against AccessNode.
//
// Design:
//   - AccessNode is a value type snapshot of element state at acquisition time.
//   - It does not hold live COM pointers.
//   - It is safe to copy across threads after construction.
//   - Provider-specific adapters are responsible for populating AccessNode fields.
//
// Threading: Immutable after construction — read from any thread.
//            Mutation only occurs during acquisition on the event thread.

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cstdint>

namespace AccessOS {

// ─── Role ────────────────────────────────────────────────────────────────────
// Normalized role — independent of UIA ControlType, MSAA role, or ARIA role.
// Providers map their native role to this enum.
enum class AccessRole {
    Unknown = 0,

    // Containers
    Window,
    Dialog,
    Pane,
    Group,
    Document,
    ScrollArea,

    // Navigation
    MenuBar,
    Menu,
    MenuItem,
    ToolBar,
    TabControl,
    Tab,
    Tree,
    TreeItem,

    // Controls
    Button,
    SplitButton,
    ToggleButton,
    CheckBox,
    RadioButton,
    ComboBox,
    ListBox,
    ListItem,
    Slider,
    Spinner,
    ProgressBar,
    ScrollBar,
    Separator,

    // Text / editing
    Edit,
    MultiLineEdit,
    PasswordEdit,
    StaticText,
    Heading,

    // Semantic web / ARIA
    Link,
    Image,
    Figure,
    Table,
    TableRow,
    TableCell,
    TableColumnHeader,
    TableRowHeader,
    List,
    ListItemRole,   // Distinct from ListItem (UIA) — maps ARIA listitem
    Form,
    FormField,
    Landmark,
    Region,
    Banner,
    Navigation,
    Main,
    Complementary,
    ContentInfo,
    Search,
    Alert,
    AlertDialog,
    Status,
    Log,
    Marquee,
    Timer,
    Tooltip,

    // Application-level
    Application,
    Desktop,
};

// ─── State ───────────────────────────────────────────────────────────────────
// Bit flags representing element state.
// Multiple states may be active simultaneously.
enum class AccessState : uint32_t {
    None         = 0,
    Focused      = 1 << 0,
    Focusable    = 1 << 1,
    Selected     = 1 << 2,
    Checked      = 1 << 3,
    Indeterminate= 1 << 4,
    Expanded     = 1 << 5,
    Collapsed    = 1 << 6,
    Pressed      = 1 << 7,
    ReadOnly     = 1 << 8,
    Required     = 1 << 9,
    Invalid      = 1 << 10,
    Busy         = 1 << 11,
    Disabled     = 1 << 12,
    Hidden       = 1 << 13,
    Offscreen    = 1 << 14,
    Protected    = 1 << 15,  // Password field — content must not be logged
    HasPopup     = 1 << 16,
    Modal        = 1 << 17,
    MultiLine    = 1 << 18,
    MultiSelect  = 1 << 19,
    Horizontal   = 1 << 20,
    Vertical     = 1 << 21,
};

inline AccessState operator|(AccessState a, AccessState b) noexcept {
    return static_cast<AccessState>(
        static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
inline AccessState operator&(AccessState a, AccessState b) noexcept {
    return static_cast<AccessState>(
        static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}
inline bool HasState(AccessState state, AccessState flag) noexcept {
    return (static_cast<uint32_t>(state) & static_cast<uint32_t>(flag)) != 0;
}

// ─── Source provider ─────────────────────────────────────────────────────────
enum class AccessProvider {
    Unknown,
    UIAutomation,
    MSAA,
    IAccessible2,
    Browser,
    ApplicationAdapter,
    OCR,
};

// ─── Bounds ──────────────────────────────────────────────────────────────────
struct AccessBounds {
    int left   = 0;
    int top    = 0;
    int width  = 0;
    int height = 0;

    bool IsEmpty() const noexcept {
        return width == 0 || height == 0;
    }
};

// ─── Heading level ───────────────────────────────────────────────────────────
// Valid values: 1–6 following HTML/ARIA heading semantics.
// 0 = not a heading.
using HeadingLevel = int;

// ─── AccessNode ──────────────────────────────────────────────────────────────
struct AccessNode {
    // ── Identity ─────────────────────────────────────────────────────────────
    // Unique identifier within a single acquisition session.
    // Not stable across process restarts or accessibility tree rebuilds.
    uint64_t        id         = 0;

    // ── Semantic properties ───────────────────────────────────────────────────
    AccessRole      role       = AccessRole::Unknown;
    std::string     name;           // Accessible name (computed)
    std::string     description;    // Accessible description
    std::string     value;          // Current value (text, slider position, etc.)
    std::string     helpText;       // Tooltip / help text
    AccessState     state      = AccessState::None;
    HeadingLevel    headingLevel = 0;

    // ── Layout ───────────────────────────────────────────────────────────────
    AccessBounds    bounds;

    // ── Text ─────────────────────────────────────────────────────────────────
    // Full text content when available (e.g. document, edit control).
    std::optional<std::string> textContent;

    // ── Hierarchy ────────────────────────────────────────────────────────────
    // Parent and children are represented as IDs.
    // The full AccessNode objects are managed by the semantic cache.
    // Using IDs avoids circular ownership and reference cycles.
    uint64_t        parentId   = 0;
    std::vector<uint64_t> childIds;

    // ── Application context ───────────────────────────────────────────────────
    std::string     applicationName;
    std::string     windowTitle;
    uint32_t        processId  = 0;

    // ── Provider information ──────────────────────────────────────────────────
    // Retained for diagnostics and inspector UI.
    // Must NOT be used for navigation or speech decisions.
    AccessProvider  provider   = AccessProvider::Unknown;

    // ── Position information ──────────────────────────────────────────────────
    // Set when the element is part of a set (e.g. list item 3 of 10).
    std::optional<int> positionInSet;
    std::optional<int> setSize;

    // ── Validity ──────────────────────────────────────────────────────────────
    // False if the underlying provider element became stale.
    bool            isValid    = true;

    // ── Convenience predicates ────────────────────────────────────────────────
    bool IsFocused()   const noexcept { return HasState(state, AccessState::Focused); }
    bool IsDisabled()  const noexcept { return HasState(state, AccessState::Disabled); }
    bool IsHidden()    const noexcept { return HasState(state, AccessState::Hidden); }
    bool IsProtected() const noexcept { return HasState(state, AccessState::Protected); }
};

} // namespace AccessOS
