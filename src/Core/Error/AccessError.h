// AccessOS/src/Core/Error/AccessError.h
//
// Structured error type for the AccessOS core.
//
// Why: Every subsystem must report failures explicitly and uniformly.
//      std::error_code is used so errors compose without exceptions
//      crossing COM/Win32 boundaries.
//
// Threading: Immutable after construction — safe to copy across threads.

#pragma once

#include <string>
#include <system_error>

namespace AccessOS {

// Error categories defined by AccessOS subsystem.
enum class ErrorCode : int {
    Success = 0,

    // COM / Windows
    ComInitializationFailed,
    ComObjectCreationFailed,

    // UI Automation
    UIAutomationUnavailable,
    UIAutomationElementInvalid,
    UIAutomationPropertyUnavailable,
    UIAutomationPatternUnavailable,

    // Semantic model
    SemanticConversionFailed,
    SemanticPropertyMissing,

    // Speech
    SpeechEngineUnavailable,
    SpeechEngineError,

    // Configuration
    ConfigurationInvalid,
    ConfigurationLoadFailed,

    // General
    NotImplemented,
    InvalidArgument,
    OperationFailed,
};

// Returns the std::error_category for AccessOS errors.
const std::error_category& AccessOSCategory() noexcept;

// Convenience factory.
inline std::error_code MakeError(ErrorCode code) noexcept {
    return { static_cast<int>(code), AccessOSCategory() };
}

// Lightweight result type used throughout the core.
// Does not use exceptions — all failures are explicit.
template<typename T>
class Result {
public:
    // Success path.
    static Result Ok(T value) {
        Result r;
        r.m_value = std::move(value);
        return r;
    }

    // Failure path.
    static Result Fail(std::error_code error, std::string message = {}) {
        Result r;
        r.m_error   = error;
        r.m_message = std::move(message);
        return r;
    }

    bool        IsOk()      const noexcept { return !m_error; }
    bool        IsError()   const noexcept { return  !!m_error; }
    const T&    Value()     const          { return m_value; }
    T&          Value()                    { return m_value; }
    std::error_code Error() const noexcept { return m_error; }
    const std::string& Message() const noexcept { return m_message; }

private:
    Result() = default;

    T               m_value{};
    std::error_code m_error{};
    std::string     m_message;
};

// Specialization for void results (operations that produce no value).
template<>
class Result<void> {
public:
    static Result Ok() {
        return Result{};
    }

    static Result Fail(std::error_code error, std::string message = {}) {
        Result r;
        r.m_error   = error;
        r.m_message = std::move(message);
        return r;
    }

    bool IsOk()    const noexcept { return !m_error; }
    bool IsError() const noexcept { return  !!m_error; }
    std::error_code Error() const noexcept { return m_error; }
    const std::string& Message() const noexcept { return m_message; }

private:
    Result() = default;

    std::error_code m_error{};
    std::string     m_message;
};

} // namespace AccessOS
