// AccessOS/src/Core/Browser/NativeMessagingHost.cpp

#include "NativeMessagingHost.h"
#include "../Logging/Logger.h"

#include <windows.h>   // SetConsoleMode / stdin binary
#include <io.h>
#include <fcntl.h>

#include <cstdint>
#include <cstring>
#include <sstream>
#include <chrono>

namespace AccessOS {

static constexpr const char* kComp = "NativeMessagingHost";

// ── Constructor ───────────────────────────────────────────────────────────────

NativeMessagingHost::NativeMessagingHost(BrowserEventCallback callback)
    : m_callback(std::move(callback))
{
}

// ── Public API ────────────────────────────────────────────────────────────────

void NativeMessagingHost::Stop() noexcept {
    m_running.store(false);
}

bool NativeMessagingHost::IsRunning() const noexcept {
    return m_running.load();
}

void NativeMessagingHost::Run() {
    // Switch stdin/stdout to binary mode — native messaging requires raw bytes.
    _setmode(_fileno(stdin),  _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);

    m_running.store(true);
    ACOS_LOG_INFO(kComp, "Native messaging host started");

    while (m_running.load()) {
        const std::string json = ReadMessage();
        if (json.empty()) {
            // EOF or error — browser closed the pipe.
            ACOS_LOG_INFO(kComp, "stdin closed — exiting");
            break;
        }

        auto evt = ParseMessage(json);
        if (evt.has_value() && m_callback) {
            m_callback(*evt);
        }
    }

    m_running.store(false);
    ACOS_LOG_INFO(kComp, "Native messaging host stopped");
}

// ── Static: read helpers ──────────────────────────────────────────────────────

bool NativeMessagingHost::ReadExact(char* buf, uint32_t size) {
    uint32_t total = 0;
    while (total < size) {
        const int n = static_cast<int>(
            fread(buf + total, 1, size - total, stdin));
        if (n <= 0) return false;
        total += static_cast<uint32_t>(n);
    }
    return true;
}

std::string NativeMessagingHost::ReadMessage() {
    // Read 4-byte little-endian length prefix.
    uint8_t lenBuf[4] = {};
    if (!ReadExact(reinterpret_cast<char*>(lenBuf), 4)) return {};

    const uint32_t length =
        static_cast<uint32_t>(lenBuf[0])        |
        (static_cast<uint32_t>(lenBuf[1]) << 8)  |
        (static_cast<uint32_t>(lenBuf[2]) << 16) |
        (static_cast<uint32_t>(lenBuf[3]) << 24);

    // Sanity cap — 1 MB max message.
    if (length == 0 || length > 1024 * 1024) {
        ACOS_LOG_WARNING(kComp, "Invalid message length: " +
            std::to_string(length));
        return {};
    }

    std::string body(length, '\0');
    if (!ReadExact(body.data(), length)) return {};
    return body;
}

// ── Static: write response ────────────────────────────────────────────────────

void NativeMessagingHost::WriteResponse(const std::string& json) {
    const uint32_t len = static_cast<uint32_t>(json.size());
    const uint8_t prefix[4] = {
        static_cast<uint8_t>(len & 0xFF),
        static_cast<uint8_t>((len >> 8)  & 0xFF),
        static_cast<uint8_t>((len >> 16) & 0xFF),
        static_cast<uint8_t>((len >> 24) & 0xFF),
    };
    fwrite(prefix, 1, 4, stdout);
    fwrite(json.c_str(), 1, len, stdout);
    fflush(stdout);
}

// ── Static: JSON parsing (minimal, no external dependency) ───────────────────
// We implement a minimal JSON field extractor rather than pulling in a full
// JSON library. The messages from the extension are well-defined and compact.

/// Extract the value of a JSON string field named `key` from a flat JSON object.
/// Only handles string values (quoted). Returns empty string if not found.
static std::string ExtractStringField(const std::string& json,
                                       const std::string& key)
{
    // Look for "key":"value"
    const std::string searchKey = "\"" + key + "\":\"";
    const size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) return {};

    const size_t valStart = keyPos + searchKey.size();
    if (valStart >= json.size()) return {};

    // Find closing quote, handling \" escapes.
    std::string result;
    result.reserve(64);
    for (size_t i = valStart; i < json.size(); ++i) {
        if (json[i] == '\\' && i + 1 < json.size()) {
            ++i;
            switch (json[i]) {
                case '"':  result += '"';  break;
                case '\\': result += '\\'; break;
                case '/':  result += '/';  break;
                case 'n':  result += '\n'; break;
                case 'r':  result += '\r'; break;
                case 't':  result += '\t'; break;
                default:   result += json[i]; break;
            }
        } else if (json[i] == '"') {
            break;
        } else {
            result += json[i];
        }
    }
    return result;
}

// ── Static: message parsing ───────────────────────────────────────────────────

std::optional<AccessEvent> NativeMessagingHost::ParseMessage(
    const std::string& json)
{
    // Extract the outer "payload" object if present (NativeMessage wrapper).
    const std::string typeStr = ExtractStringField(json, "type");

    if (typeStr == "FOCUS_CHANGED") return ParseFocusChanged(json);
    if (typeStr == "PAGE_LOADED")   return ParsePageLoaded(json);

    // Unknown type — not an error, just not consumed here.
    return std::nullopt;
}

std::optional<AccessEvent> NativeMessagingHost::ParseFocusChanged(
    const std::string& json)
{
    // Fields live inside the "node" sub-object. We extract them from the
    // flat JSON string — sufficient for our well-typed message format.
    const std::string name    = ExtractStringField(json, "name");
    const std::string roleStr = ExtractStringField(json, "role");
    const std::string pageUrl = ExtractStringField(json, "pageUrl");

    if (name.empty() && roleStr.empty()) return std::nullopt;

    AccessEvent evt;
    evt.type              = AccessEventType::FocusChanged;
    evt.element.name      = name;
    evt.element.role      = BrowserRoleToAccessRole(roleStr);
    evt.element.description = pageUrl;
    evt.element.processId = 0;
    evt.element.isValid   = true;

    // Timestamp in ms since epoch.
    evt.timestampMs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());

    return evt;
}

std::optional<AccessEvent> NativeMessagingHost::ParsePageLoaded(
    const std::string& json)
{
    const std::string url   = ExtractStringField(json, "url");
    const std::string title = ExtractStringField(json, "title");

    AccessEvent evt;
    evt.type              = AccessEventType::StructureChanged;
    evt.element.id        = 1;
    evt.element.role      = AccessRole::Document;
    evt.element.name      = title;
    evt.element.value     = url;
    evt.element.processId = 0;
    evt.element.isValid   = true;
    evt.timestampMs       = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());

    return evt;
}

// ── Static: role mapping ──────────────────────────────────────────────────────

AccessRole NativeMessagingHost::BrowserRoleToAccessRole(
    const std::string& role) noexcept
{
    static const struct { const char* key; AccessRole val; } kMap[] = {
        {"button",      AccessRole::Button},
        {"checkbox",    AccessRole::CheckBox},
        {"combobox",    AccessRole::ComboBox},
        {"document",    AccessRole::Document},
        {"edit",        AccessRole::Edit},
        {"group",       AccessRole::Group},
        {"heading",     AccessRole::Heading},
        {"image",       AccessRole::Image},
        {"link",        AccessRole::Link},
        {"listbox",     AccessRole::ListBox},
        {"listitem",    AccessRole::ListItem},
        {"menu",        AccessRole::Menu},
        {"menubar",     AccessRole::MenuBar},
        {"menuitem",    AccessRole::MenuItem},
        {"radiobutton", AccessRole::RadioButton},
        {"statictext",  AccessRole::StaticText},
        {"tab",         AccessRole::Tab},
        {"tabcontrol",  AccessRole::TabControl},
        {"tree",        AccessRole::Tree},
        {"treeitem",    AccessRole::TreeItem},
        {"window",      AccessRole::Window},
    };
    for (const auto& m : kMap) {
        if (role == m.key) return m.val;
    }
    return AccessRole::Unknown;
}

} // namespace AccessOS
