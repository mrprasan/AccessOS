// AccessOS/src/Core/Privacy/PrivacyFilter.h
//
// PrivacyFilter — enforces privacy rules at all output boundaries.
//
// Why: A screen reader has privileged access to every piece of text on screen,
//      including passwords, PINs, credit card numbers, OTPs, and auth tokens.
//      The privacy filter is the single enforcement point that prevents this
//      data from reaching speech output, log files, or the browser bridge.
//
// Rules (non-negotiable, hard-coded):
//   R1. Password fields are NEVER spoken — not the value, not a masked form.
//   R2. Password values are NEVER logged.
//   R3. Password values are NEVER forwarded to the native messaging host.
//   R4. Fields whose name matches a sensitive pattern (PIN, OTP, CVV, SSN,
//       credit card, secret, token) are treated as password fields.
//   R5. When a field IS sensitive, the spoken announcement is limited to the
//       field label + role only — e.g. "Password, edit" with no value.
//   R6. Log messages that contain sensitive values are replaced with a
//       fixed redaction marker: "[REDACTED]".
//
// Scope: This filter is INDEPENDENT of SpeechPolicy verbosity. Even at maximum
//        verbosity, sensitive values are always suppressed.
//
// Threading: All methods are stateless — safe to call from any thread.
//            No locks required.

#pragma once

#include "../Semantic/AccessNode.h"
#include <string>
#include <string_view>

namespace AccessOS {

class PrivacyFilter {
public:
    // ── Field sensitivity detection ───────────────────────────────────────────

    /// Returns true if the AccessNode represents a sensitive input field.
    /// A field is sensitive if ANY of the following is true:
    ///   - role == AccessRole::PasswordEdit
    ///   - AccessState::Protected is set
    ///   - The field name matches a sensitive keyword (see IsSensitiveName)
    static bool IsSensitiveNode(const AccessNode& node) noexcept;

    /// Returns true if the field name contains a sensitive keyword.
    /// Case-insensitive. Keywords: password, passwd, pin, otp, cvv, cvc,
    ///   ssn, social security, credit card, card number, secret, token,
    ///   auth code, verification code, security code, passphrase.
    static bool IsSensitiveName(std::string_view name) noexcept;

    /// Returns true if a URL or page context suggests a login/payment flow.
    /// Used to add extra caution in browser-sourced nodes.
    static bool IsSensitiveContext(std::string_view pageUrl) noexcept;

    // ── Speech boundary ───────────────────────────────────────────────────────

    /// Returns the spoken value for a node.
    /// If the node is sensitive, returns an empty string — always.
    /// Non-sensitive nodes return node.value unchanged.
    static std::string SpeakableValue(const AccessNode& node) noexcept;

    /// Returns the spoken name for a node.
    /// For sensitive nodes this is always safe (names are labels, not secrets).
    static std::string SpeakableName(const AccessNode& node) noexcept;

    // ── Log boundary ──────────────────────────────────────────────────────────

    /// Sanitize a log message string.
    /// Any sensitive keyword found adjacent to a value pattern is redacted.
    /// Returns the sanitized string (may be unchanged if nothing sensitive found).
    static std::string SanitizeLogMessage(std::string_view message);

    // ── Native messaging boundary ─────────────────────────────────────────────

    /// Scrub an AccessNode before forwarding to the native messaging host.
    /// If the node is sensitive, clears node.value and node.description.
    /// Returns a copy — does not modify the original.
    static AccessNode ScrubForNativeHost(const AccessNode& node) noexcept;

    // ── Redaction marker ──────────────────────────────────────────────────────

    static constexpr const char* kRedacted = "[REDACTED]";

private:
    PrivacyFilter() = delete;  // Static-only class.
};

} // namespace AccessOS
