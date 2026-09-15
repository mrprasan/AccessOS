// AccessOS/src/Core/Accessibility/UIAutomation/UIAProvider.cpp

// UIAProvider.h already pulls in <windows.h> and <uiautomation.h>
// in the correct order. Do not include uiautomation.h or
// UIAutomationCore.h again here.
#include "UIAProvider.h"
#include "UIARoleMap.h"
#include "../../Logging/Logger.h"

#include <functional>
#include <sstream>

// WRL ComPtr helpers
using Microsoft::WRL::ComPtr;

namespace AccessOS {

namespace {

static constexpr const char* kComponent = "UIAProvider";

// Converts a Windows BSTR to std::string (UTF-8 via WideCharToMultiByte).
// Returns empty string if bstr is null.
std::string BstrToString(BSTR bstr) {
    if (!bstr) return {};
    int len = ::WideCharToMultiByte(
        CP_UTF8, 0, bstr, -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    std::string result(static_cast<size_t>(len - 1), '\0');
    ::WideCharToMultiByte(
        CP_UTF8, 0, bstr, -1, result.data(), len, nullptr, nullptr);
    return result;
}

// RAII wrapper for BSTR — frees automatically on scope exit.
struct BstrGuard {
    BSTR value = nullptr;
    ~BstrGuard() { if (value) ::SysFreeString(value); }
    operator BSTR*() { return &value; }
};

// Reads a BSTR property from a UIA element safely.
// Returns empty string on failure — property read failures are not fatal.
std::string ReadBstrProperty(IUIAutomationElement* element, PROPERTYID propId) {
    if (!element) return {};
    VARIANT var{};
    ::VariantInit(&var);
    HRESULT hr = element->GetCurrentPropertyValue(propId, &var);
    if (FAILED(hr) || var.vt != VT_BSTR || !var.bstrVal) {
        ::VariantClear(&var);
        return {};
    }
    std::string result = BstrToString(var.bstrVal);
    ::VariantClear(&var);
    return result;
}

} // anonymous namespace

// ─── Constructor / Destructor ────────────────────────────────────────────────

UIAProvider::UIAProvider() = default;

UIAProvider::~UIAProvider() {
    Shutdown();
}

// ─── IAccessibilityProvider ──────────────────────────────────────────────────

Result<void> UIAProvider::Initialize() {
    if (m_initialized.load()) {
        ACOS_LOG_WARNING(kComponent, "Initialize() called on already-initialized provider");
        return Result<void>::Ok();
    }

    ACOS_LOG_INFO(kComponent, "Initializing UI Automation provider");

    // Create the IUIAutomation instance.
    // CUIAutomation8 is preferred on Windows 8+ for UIA version 4 features.
    HRESULT hr = ::CoCreateInstance(
        CLSID_CUIAutomation8,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&m_automation));

    if (FAILED(hr)) {
        // Fall back to the base CUIAutomation if CUIAutomation8 is unavailable.
        hr = ::CoCreateInstance(
            CLSID_CUIAutomation,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&m_automation));
    }

    if (FAILED(hr) || !m_automation) {
        std::ostringstream oss;
        oss << "CoCreateInstance(IUIAutomation) failed: HRESULT=0x"
            << std::hex << hr;
        ACOS_LOG_CRITICAL(kComponent, oss.str());
        return Result<void>::Fail(
            MakeError(ErrorCode::UIAutomationUnavailable),
            oss.str());
    }

    m_initialized.store(true);
    ACOS_LOG_INFO(kComponent, "UI Automation provider initialized successfully");
    return Result<void>::Ok();
}

void UIAProvider::Shutdown() {
    if (!m_initialized.exchange(false)) return;

    ACOS_LOG_INFO(kComponent, "Shutting down UI Automation provider");
    m_automation.Reset();
}

AccessProvider UIAProvider::ProviderType() const noexcept {
    return AccessProvider::UIAutomation;
}

bool UIAProvider::IsAvailable() const noexcept {
    return m_initialized.load() && m_automation != nullptr;
}

Result<AccessNode> UIAProvider::GetFocusedElement() {
    if (!IsAvailable()) {
        return Result<AccessNode>::Fail(
            MakeError(ErrorCode::UIAutomationUnavailable),
            "Provider not initialized");
    }

    ComPtr<IUIAutomationElement> element;
    HRESULT hr = m_automation->GetFocusedElement(&element);

    if (FAILED(hr) || !element) {
        std::ostringstream oss;
        oss << "GetFocusedElement failed: HRESULT=0x" << std::hex << hr;
        ACOS_LOG_WARNING(kComponent, oss.str());
        return Result<AccessNode>::Fail(
            MakeError(ErrorCode::UIAutomationElementInvalid),
            oss.str());
    }

    return ElementToNode(element.Get());
}

Result<AccessNode> UIAProvider::GetRootElement() {
    if (!IsAvailable()) {
        return Result<AccessNode>::Fail(
            MakeError(ErrorCode::UIAutomationUnavailable),
            "Provider not initialized");
    }

    ComPtr<IUIAutomationElement> root;
    HRESULT hr = m_automation->GetRootElement(&root);

    if (FAILED(hr) || !root) {
        std::ostringstream oss;
        oss << "GetRootElement failed: HRESULT=0x" << std::hex << hr;
        ACOS_LOG_ERROR(kComponent, oss.str());
        return Result<AccessNode>::Fail(
            MakeError(ErrorCode::UIAutomationElementInvalid),
            oss.str());
    }

    return ElementToNode(root.Get());
}

Result<AccessNode> UIAProvider::GetElementAtPoint(int x, int y) {
    if (!IsAvailable()) {
        return Result<AccessNode>::Fail(
            MakeError(ErrorCode::UIAutomationUnavailable),
            "Provider not initialized");
    }

    POINT pt{ x, y };
    ComPtr<IUIAutomationElement> element;
    HRESULT hr = m_automation->ElementFromPoint(pt, &element);

    if (FAILED(hr) || !element) {
        std::ostringstream oss;
        oss << "ElementFromPoint(" << x << "," << y
            << ") failed: HRESULT=0x" << std::hex << hr;
        ACOS_LOG_WARNING(kComponent, oss.str());
        return Result<AccessNode>::Fail(
            MakeError(ErrorCode::UIAutomationElementInvalid),
            oss.str());
    }

    return ElementToNode(element.Get());
}

Result<AccessNode> UIAProvider::GetParent(uint64_t /*elementId*/) {
    // NOT IMPLEMENTED — requires element cache keyed by ID.
    // Will be implemented in ACCESSOS-004 (Semantic Model / element cache).
    return Result<AccessNode>::Fail(
        MakeError(ErrorCode::NotImplemented),
        "GetParent requires element cache — ACCESSOS-004");
}

Result<std::vector<AccessNode>> UIAProvider::GetChildren(uint64_t /*elementId*/) {
    // NOT IMPLEMENTED — requires element cache keyed by ID.
    // Will be implemented in ACCESSOS-004 (Semantic Model / element cache).
    return Result<std::vector<AccessNode>>::Fail(
        MakeError(ErrorCode::NotImplemented),
        "GetChildren requires element cache — ACCESSOS-004");
}

void UIAProvider::SetFocusChangedCallback(FocusChangedCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_focusCallback = std::move(callback);
}

void UIAProvider::SetElementChangedCallback(ElementChangedCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_elementCallback = std::move(callback);
}

// ─── Private helpers ──────────────────────────────────────────────────────────

Result<AccessNode> UIAProvider::ElementToNode(IUIAutomationElement* element) {
    if (!element) {
        return Result<AccessNode>::Fail(
            MakeError(ErrorCode::UIAutomationElementInvalid),
            "Null element passed to ElementToNode");
    }

    AccessNode node;
    node.provider = AccessProvider::UIAutomation;
    node.isValid  = true;

    // ── Role ─────────────────────────────────────────────────────────────────
    CONTROLTYPEID ctid = 0;
    if (SUCCEEDED(element->get_CurrentControlType(&ctid))) {
        node.role = UIARoleMap::FromControlType(ctid);
    }

    // ── Name ─────────────────────────────────────────────────────────────────
    // UIA Name property is the primary accessible name source.
    {
        BstrGuard bstr;
        if (SUCCEEDED(element->get_CurrentName(bstr))) {
            node.name = BstrToString(bstr.value);
        }
    }

    // ── Description / HelpText ────────────────────────────────────────────────
    {
        BstrGuard bstr;
        if (SUCCEEDED(element->get_CurrentHelpText(bstr))) {
            node.helpText = BstrToString(bstr.value);
        }
    }

    // ── Value ────────────────────────────────────────────────────────────────
    // Read value from ValuePattern if available; do not fail if absent.
    {
        ComPtr<IUIAutomationValuePattern> valuePattern;
        HRESULT hr = element->GetCurrentPatternAs(
            UIA_ValuePatternId,
            IID_PPV_ARGS(&valuePattern));
        if (SUCCEEDED(hr) && valuePattern) {
            BstrGuard bstr;
            if (SUCCEEDED(valuePattern->get_CurrentValue(bstr))) {
                node.value = BstrToString(bstr.value);
            }
        }
    }

    // ── State ────────────────────────────────────────────────────────────────
    node.state = ReadState(element);

    // ── Bounds ───────────────────────────────────────────────────────────────
    RECT rect{};
    if (SUCCEEDED(element->get_CurrentBoundingRectangle(&rect))) {
        node.bounds.left   = rect.left;
        node.bounds.top    = rect.top;
        node.bounds.width  = rect.right  - rect.left;
        node.bounds.height = rect.bottom - rect.top;
    }

    // ── Application context ───────────────────────────────────────────────────
    {
        BstrGuard bstr;
        if (SUCCEEDED(element->get_CurrentFrameworkId(bstr))) {
            // FrameworkId is used internally for diagnostics — not exposed as name.
            (void)bstr; // Suppress unused warning
        }
    }

    // Process ID — used for application identification.
    {
        int pid = 0;
        if (SUCCEEDED(element->get_CurrentProcessId(&pid))) {
            node.processId = static_cast<uint32_t>(pid);
        }
    }

    // ── Element ID ───────────────────────────────────────────────────────────
    node.id = ComputeElementId(element);

    return Result<AccessNode>::Ok(std::move(node));
}

AccessState UIAProvider::ReadState(IUIAutomationElement* element) {
    AccessState state = AccessState::None;
    if (!element) return state;

    // Focusable
    BOOL canFocus = FALSE;
    if (SUCCEEDED(element->get_CurrentIsKeyboardFocusable(&canFocus)) && canFocus) {
        state = state | AccessState::Focusable;
    }

    // Focused
    BOOL hasFocus = FALSE;
    if (SUCCEEDED(element->get_CurrentHasKeyboardFocus(&hasFocus)) && hasFocus) {
        state = state | AccessState::Focused;
    }

    // Enabled / Disabled
    BOOL enabled = TRUE;
    if (SUCCEEDED(element->get_CurrentIsEnabled(&enabled)) && !enabled) {
        state = state | AccessState::Disabled;
    }

    // Offscreen
    BOOL offscreen = FALSE;
    if (SUCCEEDED(element->get_CurrentIsOffscreen(&offscreen)) && offscreen) {
        state = state | AccessState::Offscreen;
    }

    // Password field — mark as Protected so logging layer redacts content.
    BOOL isPassword = FALSE;
    if (SUCCEEDED(element->get_CurrentIsPassword(&isPassword)) && isPassword) {
        state = state | AccessState::Protected;
    }

    // Required (AriaProperties) — best effort
    // Toggle state via TogglePattern
    {
        ComPtr<IUIAutomationTogglePattern> toggle;
        if (SUCCEEDED(element->GetCurrentPatternAs(
                UIA_TogglePatternId, IID_PPV_ARGS(&toggle))) && toggle) {
            ToggleState ts = ToggleState_Off;
            if (SUCCEEDED(toggle->get_CurrentToggleState(&ts))) {
                if (ts == ToggleState_On)          state = state | AccessState::Checked;
                if (ts == ToggleState_Indeterminate) state = state | AccessState::Indeterminate;
            }
        }
    }

    // Expand/Collapse state via ExpandCollapsePattern
    {
        ComPtr<IUIAutomationExpandCollapsePattern> ec;
        if (SUCCEEDED(element->GetCurrentPatternAs(
                UIA_ExpandCollapsePatternId, IID_PPV_ARGS(&ec))) && ec) {
            ExpandCollapseState ecs = ExpandCollapseState_Collapsed;
            if (SUCCEEDED(ec->get_CurrentExpandCollapseState(&ecs))) {
                if (ecs == ExpandCollapseState_Expanded)   state = state | AccessState::Expanded;
                if (ecs == ExpandCollapseState_Collapsed)  state = state | AccessState::Collapsed;
            }
        }
    }

    return state;
}

uint64_t UIAProvider::ComputeElementId(IUIAutomationElement* element) {
    if (!element) return 0;

    // UIA RuntimeId is an array of ints that uniquely identifies an element
    // within the current desktop session. We hash it to a uint64_t for our ID.
    SAFEARRAY* runtimeId = nullptr;
    if (FAILED(element->GetRuntimeId(&runtimeId)) || !runtimeId) return 0;

    uint64_t hash = 14695981039346656037ULL; // FNV-1a offset basis
    constexpr uint64_t fnvPrime = 1099511628211ULL;

    LONG lBound = 0, uBound = 0;
    ::SafeArrayGetLBound(runtimeId, 1, &lBound);
    ::SafeArrayGetUBound(runtimeId, 1, &uBound);

    for (LONG i = lBound; i <= uBound; ++i) {
        int val = 0;
        ::SafeArrayGetElement(runtimeId, &i, &val);
        // FNV-1a hash over each byte of the int
        const auto* bytes = reinterpret_cast<const uint8_t*>(&val);
        for (int b = 0; b < 4; ++b) {
            hash ^= bytes[b];
            hash *= fnvPrime;
        }
    }

    ::SafeArrayDestroy(runtimeId);
    return hash;
}

AccessRole UIAProvider::ControlTypeToRole(CONTROLTYPEID controlType) noexcept {
    return UIARoleMap::FromControlType(controlType);
}

std::string UIAProvider::ReadName(IUIAutomationElement* element) {
    if (!element) return {};
    BstrGuard bstr;
    if (SUCCEEDED(element->get_CurrentName(bstr))) {
        return BstrToString(bstr.value);
    }
    return {};
}

} // namespace AccessOS
