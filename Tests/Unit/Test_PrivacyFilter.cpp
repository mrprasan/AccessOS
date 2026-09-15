// Tests/Unit/Test_PrivacyFilter.cpp
//
// Unit tests for PrivacyFilter.
// No live UIA, no speech, no network required.
// Verifies every privacy rule (R1–R6) exhaustively.

#include <gtest/gtest.h>
#include "Privacy/PrivacyFilter.h"
#include "Semantic/AccessNode.h"

using namespace AccessOS;

// ─── Helper builders ──────────────────────────────────────────────────────────

static AccessNode MakeNode(AccessRole role,
                            const std::string& name,
                            const std::string& value = "",
                            AccessState state = AccessState::None,
                            const std::string& desc = "") {
    AccessNode n;
    n.role        = role;
    n.name        = name;
    n.value       = value;
    n.state       = state;
    n.description = desc;
    n.isValid     = true;
    return n;
}

static AccessNode PasswordNode(const std::string& name = "Password",
                                const std::string& value = "secret123") {
    return MakeNode(AccessRole::PasswordEdit, name, value,
                    AccessState::Protected | AccessState::Focused);
}

static AccessNode EditNode(const std::string& name = "Username",
                            const std::string& value = "alice") {
    return MakeNode(AccessRole::Edit, name, value);
}

// ─── IsSensitiveNode ──────────────────────────────────────────────────────────

TEST(PrivacyFilterTest, PasswordEditRoleIsSensitive) {
    EXPECT_TRUE(PrivacyFilter::IsSensitiveNode(PasswordNode()));
}

TEST(PrivacyFilterTest, ProtectedStateIsSensitive) {
    auto n = EditNode();
    n.state = AccessState::Protected;
    EXPECT_TRUE(PrivacyFilter::IsSensitiveNode(n));
}

TEST(PrivacyFilterTest, PlainEditNodeIsNotSensitive) {
    EXPECT_FALSE(PrivacyFilter::IsSensitiveNode(EditNode()));
}

TEST(PrivacyFilterTest, ButtonIsNotSensitive) {
    auto n = MakeNode(AccessRole::Button, "Submit", "");
    EXPECT_FALSE(PrivacyFilter::IsSensitiveNode(n));
}

// ─── IsSensitiveName — keyword coverage ──────────────────────────────────────

struct NameCase { const char* name; bool expected; };

class SensitiveNameTest : public testing::TestWithParam<NameCase> {};

TEST_P(SensitiveNameTest, MatchesExpected) {
    auto [name, expected] = GetParam();
    EXPECT_EQ(PrivacyFilter::IsSensitiveName(name), expected)
        << "name=" << name;
}

INSTANTIATE_TEST_SUITE_P(Keywords, SensitiveNameTest, testing::Values(
    NameCase{"Password",              true},
    NameCase{"Current password",      true},
    NameCase{"passwd",                true},
    NameCase{"Enter your PIN",        true},
    NameCase{"OTP",                   true},
    NameCase{"One-time code",         true},
    NameCase{"CVV",                   true},
    NameCase{"CVC",                   true},
    NameCase{"Social Security Number",true},
    NameCase{"Credit Card Number",    true},
    NameCase{"cardnumber",            true},
    NameCase{"Secret key",            true},
    NameCase{"API Token",             true},
    NameCase{"Auth code",             true},
    NameCase{"Verification Code",     true},
    NameCase{"Security Code",         true},
    NameCase{"Passphrase",            true},
    NameCase{"Private Key",           true},
    NameCase{"API key",               true},
    // Non-sensitive names
    NameCase{"Username",              false},
    NameCase{"Email address",         false},
    NameCase{"Search",                false},
    NameCase{"Submit",                false},
    NameCase{"First name",            false},
    NameCase{"",                      false}
));

// ─── IsSensitiveName — case insensitivity ─────────────────────────────────────

TEST(PrivacyFilterTest, SensitiveNameCaseInsensitive) {
    EXPECT_TRUE(PrivacyFilter::IsSensitiveName("PASSWORD"));
    EXPECT_TRUE(PrivacyFilter::IsSensitiveName("pAsSwOrD"));
    EXPECT_TRUE(PrivacyFilter::IsSensitiveName("CrEdIt CaRd NuMbEr"));
}

// ─── Name from node description ───────────────────────────────────────────────

TEST(PrivacyFilterTest, SensitiveDescriptionMakesNodeSensitive) {
    auto n = MakeNode(AccessRole::Edit, "Field", "value", AccessState::None,
                      "Enter your password here");
    EXPECT_TRUE(PrivacyFilter::IsSensitiveNode(n));
}

// ─── IsSensitiveContext ───────────────────────────────────────────────────────

struct UrlCase { const char* url; bool expected; };
class SensitiveContextTest : public testing::TestWithParam<UrlCase> {};

TEST_P(SensitiveContextTest, MatchesExpected) {
    auto [url, expected] = GetParam();
    EXPECT_EQ(PrivacyFilter::IsSensitiveContext(url), expected)
        << "url=" << url;
}

INSTANTIATE_TEST_SUITE_P(Urls, SensitiveContextTest, testing::Values(
    UrlCase{"https://example.com/login",      true},
    UrlCase{"https://bank.com/signin",        true},
    UrlCase{"https://shop.com/checkout",      true},
    UrlCase{"https://shop.com/payment",       true},
    UrlCase{"https://accounts.google.com/auth", true},
    UrlCase{"https://example.com/2fa",        true},
    UrlCase{"https://example.com/verify",     true},
    UrlCase{"https://example.com/reset",      true},
    // Non-sensitive
    UrlCase{"https://example.com/about",      false},
    UrlCase{"https://news.com/article/123",   false},
    UrlCase{"https://example.com/search",     false},
    UrlCase{"",                               false}
));

// ─── R1: SpeakableValue — password value is NEVER spoken ─────────────────────

TEST(PrivacyFilterTest, R1_PasswordValueNeverSpoken) {
    // Role-based detection.
    auto n = PasswordNode("Password", "MySecret123!");
    EXPECT_EQ(PrivacyFilter::SpeakableValue(n), "");
}

TEST(PrivacyFilterTest, R1_ProtectedStateValueNeverSpoken) {
    auto n = EditNode();
    n.state = AccessState::Protected;
    n.value = "hidden_value";
    EXPECT_EQ(PrivacyFilter::SpeakableValue(n), "");
}

TEST(PrivacyFilterTest, R1_SensitiveNameFieldValueNeverSpoken) {
    auto n = MakeNode(AccessRole::Edit, "Enter your PIN", "1234");
    EXPECT_EQ(PrivacyFilter::SpeakableValue(n), "");
}

TEST(PrivacyFilterTest, R1_NonSensitiveValueSpokenNormally) {
    auto n = EditNode("Username", "alice");
    EXPECT_EQ(PrivacyFilter::SpeakableValue(n), "alice");
}

TEST(PrivacyFilterTest, R1_EmptyValueReturnedForSensitive) {
    auto n = PasswordNode("Password", "");
    EXPECT_EQ(PrivacyFilter::SpeakableValue(n), "");
}

// ─── SpeakableName — label is always safe ─────────────────────────────────────

TEST(PrivacyFilterTest, PasswordLabelIsSafeToSpeak) {
    auto n = PasswordNode("Password", "secret");
    EXPECT_EQ(PrivacyFilter::SpeakableName(n), "Password");
}

TEST(PrivacyFilterTest, UsernameLabelIsSafeToSpeak) {
    auto n = EditNode("Username", "alice");
    EXPECT_EQ(PrivacyFilter::SpeakableName(n), "Username");
}

// ─── R2/R6: SanitizeLogMessage ────────────────────────────────────────────────

TEST(PrivacyFilterTest, R2_PasswordInLogIsRedacted) {
    const std::string msg = "User typed password: hunter2";
    const std::string result = PrivacyFilter::SanitizeLogMessage(msg);
    EXPECT_EQ(result.find("hunter2"), std::string::npos)
        << "Actual sensitive value must not appear: " << result;
    EXPECT_NE(result.find(PrivacyFilter::kRedacted), std::string::npos);
}

TEST(PrivacyFilterTest, R6_SafeMessageUnchanged) {
    const std::string msg = "Focus changed to Button: Submit";
    EXPECT_EQ(PrivacyFilter::SanitizeLogMessage(msg), msg);
}

TEST(PrivacyFilterTest, R6_EmptyMessageUnchanged) {
    EXPECT_EQ(PrivacyFilter::SanitizeLogMessage(""), "");
}

TEST(PrivacyFilterTest, R6_TokenInLogIsRedacted) {
    const std::string msg = "auth token=eyJhbGciOiJIUzI1NiJ9";
    const std::string result = PrivacyFilter::SanitizeLogMessage(msg);
    EXPECT_EQ(result.find("eyJhbGciOiJIUzI1NiJ9"), std::string::npos)
        << "Auth token must be redacted: " << result;
}

TEST(PrivacyFilterTest, R6_MultipleKeywordsRedacted) {
    const std::string msg = "password:abc otp:123456";
    const std::string result = PrivacyFilter::SanitizeLogMessage(msg);
    EXPECT_EQ(result.find("abc"),    std::string::npos);
    EXPECT_EQ(result.find("123456"), std::string::npos);
}

// ─── R3: ScrubForNativeHost ───────────────────────────────────────────────────

TEST(PrivacyFilterTest, R3_PasswordNodeScrubbed) {
    auto n = PasswordNode("Password", "s3cr3t");
    const auto scrubbed = PrivacyFilter::ScrubForNativeHost(n);
    EXPECT_TRUE(scrubbed.value.empty())     << "Value must be cleared";
    EXPECT_EQ(scrubbed.name, "Password")   << "Name must be preserved";
}

TEST(PrivacyFilterTest, R3_NonSensitiveNodeUnchanged) {
    auto n = MakeNode(AccessRole::Button, "Submit", "Submit");
    const auto scrubbed = PrivacyFilter::ScrubForNativeHost(n);
    EXPECT_EQ(scrubbed.value, "Submit");
    EXPECT_EQ(scrubbed.name,  "Submit");
}

TEST(PrivacyFilterTest, R3_PageUrlPreservedAfterScrub) {
    auto n = PasswordNode("Password", "secret");
    n.description = "https://example.com/login";
    const auto scrubbed = PrivacyFilter::ScrubForNativeHost(n);
    // URL starts with "http" — should be preserved.
    EXPECT_EQ(scrubbed.description, "https://example.com/login");
    EXPECT_TRUE(scrubbed.value.empty());
}

TEST(PrivacyFilterTest, R3_NonUrlDescriptionClearedAfterScrub) {
    auto n = PasswordNode("Password", "secret");
    n.description = "Enter your secret password here";
    const auto scrubbed = PrivacyFilter::ScrubForNativeHost(n);
    EXPECT_TRUE(scrubbed.description.empty());
}

// ─── Redaction marker ─────────────────────────────────────────────────────────

TEST(PrivacyFilterTest, RedactedConstantIsCorrect) {
    EXPECT_STREQ(PrivacyFilter::kRedacted, "[REDACTED]");
}
