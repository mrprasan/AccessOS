// Tests/Unit/Test_AccessError.cpp
// Unit tests for AccessOS::Result and AccessOS::ErrorCode.

#include <gtest/gtest.h>
#include "Error/AccessError.h"

using namespace AccessOS;

// ─── Result<T> ───────────────────────────────────────────────────────────────

TEST(ResultTest, OkResultIsOk) {
    auto result = Result<int>::Ok(42);
    EXPECT_TRUE(result.IsOk());
    EXPECT_FALSE(result.IsError());
    EXPECT_EQ(result.Value(), 42);
}

TEST(ResultTest, FailResultIsError) {
    auto result = Result<int>::Fail(
        MakeError(ErrorCode::UIAutomationUnavailable),
        "UIA not available"
    );
    EXPECT_FALSE(result.IsOk());
    EXPECT_TRUE(result.IsError());
    EXPECT_EQ(result.Error(), MakeError(ErrorCode::UIAutomationUnavailable));
    EXPECT_EQ(result.Message(), "UIA not available");
}

TEST(ResultTest, VoidOkResultIsOk) {
    auto result = Result<void>::Ok();
    EXPECT_TRUE(result.IsOk());
    EXPECT_FALSE(result.IsError());
}

TEST(ResultTest, VoidFailResultIsError) {
    auto result = Result<void>::Fail(
        MakeError(ErrorCode::ComInitializationFailed),
        "COM failed"
    );
    EXPECT_FALSE(result.IsOk());
    EXPECT_TRUE(result.IsError());
    EXPECT_EQ(result.Error(), MakeError(ErrorCode::ComInitializationFailed));
}

// ─── Error category ──────────────────────────────────────────────────────────

TEST(ErrorCategoryTest, CategoryNameIsAccessOS) {
    EXPECT_STREQ(AccessOSCategory().name(), "AccessOS");
}

TEST(ErrorCategoryTest, SuccessMessageIsCorrect) {
    EXPECT_EQ(
        AccessOSCategory().message(static_cast<int>(ErrorCode::Success)),
        "Success"
    );
}

TEST(ErrorCategoryTest, UnknownCodeReturnsUnknownError) {
    EXPECT_EQ(
        AccessOSCategory().message(9999),
        "Unknown error"
    );
}

TEST(ErrorCategoryTest, MakeErrorProducesCorrectCode) {
    auto ec = MakeError(ErrorCode::UIAutomationElementInvalid);
    EXPECT_EQ(ec.value(), static_cast<int>(ErrorCode::UIAutomationElementInvalid));
    EXPECT_EQ(&ec.category(), &AccessOSCategory());
}

TEST(ErrorCategoryTest, DifferentErrorCodesAreNotEqual) {
    EXPECT_NE(
        MakeError(ErrorCode::UIAutomationUnavailable),
        MakeError(ErrorCode::ComInitializationFailed)
    );
}
