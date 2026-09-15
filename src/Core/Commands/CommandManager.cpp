// AccessOS/src/Core/Commands/CommandManager.cpp

#include "CommandManager.h"
#include "../Logging/Logger.h"

namespace AccessOS {

void CommandManager::Register(std::shared_ptr<ICommand> command) {
    if (!command) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_commands[command->Id()] = std::move(command);
}

void CommandManager::Register(std::string id, std::string displayName,
                               std::function<void()> fn)
{
    Register(std::make_shared<LambdaCommand>(
        std::move(id), std::move(displayName), std::move(fn)));
}

bool CommandManager::Execute(const std::string& commandId) {
    std::shared_ptr<ICommand> cmd;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_commands.find(commandId);
        if (it == m_commands.end()) {
            ACOS_LOG_WARNING("CommandManager",
                "Unknown command: " + commandId);
            return false;
        }
        cmd = it->second;
    }
    cmd->Execute();
    return true;
}

ICommand* CommandManager::Get(const std::string& commandId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_commands.find(commandId);
    if (it == m_commands.end()) return nullptr;
    return it->second.get();
}

std::vector<std::string> CommandManager::AllIds() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> ids;
    ids.reserve(m_commands.size());
    for (const auto& [id, _] : m_commands) {
        ids.push_back(id);
    }
    return ids;
}

size_t CommandManager::Count() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_commands.size();
}

} // namespace AccessOS
