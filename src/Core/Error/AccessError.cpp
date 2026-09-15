// AccessOS/src/Core/Error/AccessError.cpp

#include "AccessError.h"

namespace AccessOS {

namespace {

class AccessOSErrorCategory final : public std::error_category {
public:
    const char* name() const noexcept override {
        return "AccessOS";
    }

    std::string message(int code) const override {
        switch (static_cast<ErrorCode>(code)) {
        case ErrorCode::Success:                        return "Success";
        case ErrorCode::ComInitializationFailed:        return "COM initialization failed";
        case ErrorCode::ComObjectCreationFailed:        return "COM object creation failed";
        case ErrorCode::UIAutomationUnavailable:        return "UI Automation unavailable";
        case ErrorCode::UIAutomationElementInvalid:     return "UI Automation element invalid or stale";
        case ErrorCode::UIAutomationPropertyUnavailable:return "UI Automation property unavailable";
        case ErrorCode::UIAutomationPatternUnavailable: return "UI Automation pattern unavailable";
        case ErrorCode::SemanticConversionFailed:       return "Semantic conversion failed";
        case ErrorCode::SemanticPropertyMissing:        return "Semantic property missing";
        case ErrorCode::SpeechEngineUnavailable:        return "Speech engine unavailable";
        case ErrorCode::SpeechEngineError:              return "Speech engine error";
        case ErrorCode::ConfigurationInvalid:           return "Configuration invalid";
        case ErrorCode::ConfigurationLoadFailed:        return "Configuration load failed";
        case ErrorCode::NotImplemented:                 return "Not implemented";
        case ErrorCode::InvalidArgument:                return "Invalid argument";
        case ErrorCode::OperationFailed:                return "Operation failed";
        default:                                        return "Unknown error";
        }
    }
};

} // anonymous namespace

const std::error_category& AccessOSCategory() noexcept {
    static const AccessOSErrorCategory instance;
    return instance;
}

} // namespace AccessOS
