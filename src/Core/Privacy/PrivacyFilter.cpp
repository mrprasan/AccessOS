// AccessOS/src/Core/Privacy/PrivacyFilter.cpp

#include "PrivacyFilter.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace AccessOS {

// ── Sensitive keyword table ───────────────────────────────────────────────────
// Each entry is a lowercase substring to search for in field names/messages.
// Ordered roughly by frequency to short-circuit early.

static constexpr const char* kSensitiveKeywords[] = {
    "password",
    "passwd",
    " pin",
    "pin ",
    "pin:",
    "\tpin",
    "otp",
    "one-time",
    "one time",
    "cvv",
    "cvc",
    "ssn",
    "social security",
    "credit card",
    "card number",
    "cardnumber",
    "secret",
    "token",
    "auth code",
    "authcode",
    "verification code",
    "security code",
    "passphrase",
    "pass phrase",
    "private key",
    "api key",
    "apikey",
};

// Sensitive URL fragments — indicates a login or payment context.
static constexpr const char* kSensitiveUrlFragments[] = {
    "login",
    "signin",
    "sign-in",
    "password",
    "auth",
    "checkout",
    "payment",
    "billing",
    "account/security",
    "2fa",
    "mfa",
    "verify",
    "recover",
    "reset",
};

// ── Helpers ───────────────────────────────────────────────────────────────────

/// Convert string_view to lowercase for case-insensitive comparison.
static std::string ToLower(std::string_view sv) {
    std::string result(sv);
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

/// Returns true if haystack contains needle (both lowercase).
static bool ContainsLower(const std::string& haystack,
                           const char* needle) noexcept {
    return haystack.find(needle) != std::string::npos;
}

// ── IsSensitiveName ───────────────────────────────────────────────────────────

bool PrivacyFilter::IsSensitiveName(std::string_view name) noexcept {
    if (name.empty()) return false;
    const std::string lower = ToLower(name);
    for (const char* kw : kSensitiveKeywords) {
        if (ContainsLower(lower, kw)) return true;
    }
    return false;
}

// ── IsSensitiveContext ────────────────────────────────────────────────────────

bool PrivacyFilter::IsSensitiveContext(std::string_view pageUrl) noexcept {
    if (pageUrl.empty()) return false;
    const std::string lower = ToLower(pageUrl);
    for (const char* frag : kSensitiveUrlFragments) {
        if (ContainsLower(lower, frag)) return true;
    }
    return false;
}

// ── IsSensitiveNode ───────────────────────────────────────────────────────────

bool PrivacyFilter::IsSensitiveNode(const AccessNode& node) noexcept {
    // Rule: explicit PasswordEdit role.
    if (node.role == AccessRole::PasswordEdit) return true;

    // Rule: Protected state flag.
    if (HasState(node.state, AccessState::Protected)) return true;

    // Rule: field name matches a sensitive keyword.
    if (IsSensitiveName(node.name)) return true;

    // Rule: description contains a sensitive keyword (e.g. aria-description).
    if (IsSensitiveName(node.description)) return true;

    return false;
}

// ── SpeakableValue ────────────────────────────────────────────────────────────

std::string PrivacyFilter::SpeakableValue(const AccessNode& node) noexcept {
    // Rule R1: sensitive fields return empty — never speak the value.
    if (IsSensitiveNode(node)) return "";
    return node.value;
}

// ── SpeakableName ─────────────────────────────────────────────────────────────

std::string PrivacyFilter::SpeakableName(const AccessNode& node) noexcept {
    // Names (labels) are safe — they are authored by the developer, not
    // entered by the user. Return unchanged.
    return node.name;
}

// ── SanitizeLogMessage ────────────────────────────────────────────────────────
//
// Strategy: find any sensitive keyword in the message; if found, replace
// the entire portion from the keyword to the next whitespace/end-of-line
// with [REDACTED]. This is a conservative approach — it may redact too much,
// but it will never redact too little.

std::string PrivacyFilter::SanitizeLogMessage(std::string_view message) {
    if (message.empty()) return {};

    std::string result(message);

    for (const char* kw : kSensitiveKeywords) {
        const size_t kwLen = std::char_traits<char>::length(kw);

        // Rebuild lower from current result each iteration so positions stay
        // correct even after earlier replacements changed the string length.
        const std::string lower = ToLower(result);

        const size_t pos = lower.find(kw);
        if (pos == std::string::npos) continue;

        // Skip separator characters between keyword and value.
        size_t valStart = pos + kwLen;
        while (valStart < result.size() &&
               (result[valStart] == ':' || result[valStart] == '=' ||
                result[valStart] == ' ' || result[valStart] == '\t' ||
                result[valStart] == '"' || result[valStart] == '\'')) {
            ++valStart;
        }

        // Locate the end of the value token.
        size_t valEnd = valStart;
        while (valEnd < result.size() &&
               result[valEnd] != ' ' && result[valEnd] != '\t' &&
               result[valEnd] != '\n' && result[valEnd] != '\r' &&
               result[valEnd] != '"' && result[valEnd] != '\'' &&
               result[valEnd] != ',') {
            ++valEnd;
        }

        if (valEnd > valStart) {
            result.replace(valStart, valEnd - valStart, kRedacted);
        }
    }

    return result;
}

// ── ScrubForNativeHost ────────────────────────────────────────────────────────

AccessNode PrivacyFilter::ScrubForNativeHost(const AccessNode& node) noexcept {
    if (!IsSensitiveNode(node)) return node;

    // Rule R3: clear value and description — name (label) is kept.
    AccessNode scrubbed = node;
    scrubbed.value.clear();
    // Keep description if it is the page URL (starts with http), clear otherwise.
    if (scrubbed.description.find("http") != 0) {
        scrubbed.description.clear();
    }
    return scrubbed;
}

} // namespace AccessOS
