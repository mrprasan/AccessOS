// AccessOS/src/Core/Commands/CommandManager.h
//
// CommandManager — registry of all available commands.
//
// Why: Commands must be registered once, then looked up by ID
//      from any subsystem (keyboard, UI, scripting). A central
//      registry prevents duplicate command definitions and enables
//      testability without a physical keyboard.
//
// Threading: Register() is called during initialization (single-threaded).
//            Execute() and Get() are thread-safe.

#pragma once

#include "ICommand.h"

#include <memory>
#include <unordered_map>
#include <string>
#include <mutex>
#include <vector>
#include <functional>

namespace AccessOS {

// Convenience: create a command from a lambda with no boilerplate.
class LambdaCommand final : public ICommand {
public:
    LambdaCommand(std::string id, std::string name,
                  std::function<void()> fn)
        : m_id(std::move(id))
        , m_name(std::move(name))
        , m_fn(std::move(fn))
    {}

    void        Execute()              override { if (m_fn) m_fn(); }
    const char* Id()          const noexcept override { return m_id.c_str(); }
    const char* DisplayName() const noexcept override { return m_name.c_str(); }

private:
    std::string           m_id;
    std::string           m_name;
    std::function<void()> m_fn;
};

class CommandManager {
public:
    CommandManager()  = default;
    ~CommandManager() = default;

    // Register a command. Replaces any existing command with the same ID.
    // Thread-safe after initialization.
    void Register(std::shared_ptr<ICommand> command);

    // Convenience: register a lambda as a command.
    void Register(std::string id, std::string displayName,
                  std::function<void()> fn);

    // Execute a command by ID. Returns false if the ID is not registered.
    bool Execute(const std::string& commandId);

    // Returns a command by ID, or nullptr if not found.
    ICommand* Get(const std::string& commandId) const;

    // Returns all registered command IDs.
    std::vector<std::string> AllIds() const;

    // Returns the number of registered commands.
    size_t Count() const;

private:
    mutable std::mutex                                  m_mutex;
    std::unordered_map<std::string,
                       std::shared_ptr<ICommand>>       m_commands;
};

} // namespace AccessOS
