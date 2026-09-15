// AccessOS/src/Core/Commands/ICommand.h
//
// Command interface — represents a single executable action.
//
// Why: All keyboard-triggered actions must be decoupled from the
//      key that triggers them. Commands are testable without a
//      physical keyboard and can be bound to multiple shortcuts.
//
// Threading: Execute() is called on the command dispatch thread.
//            Implementations must not block the keyboard callback.

#pragma once

#include <string>

namespace AccessOS {

class ICommand {
public:
    virtual ~ICommand() = default;

    // Execute the command. Called on the command dispatch thread.
    virtual void Execute() = 0;

    // Unique command identifier (e.g. "navigation.next", "speech.stop").
    virtual const char* Id() const noexcept = 0;

    // Human-readable name for UI display.
    virtual const char* DisplayName() const noexcept = 0;
};

} // namespace AccessOS
