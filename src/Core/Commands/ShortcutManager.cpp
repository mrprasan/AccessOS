// AccessOS/src/Core/Commands/ShortcutManager.cpp

#include "ShortcutManager.h"

namespace AccessOS {

bool ShortcutManager::Bind(KeyStroke key, std::string commandId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_bindings.count(key)) return false;   // Conflict
    m_bindings[key] = std::move(commandId);
    return true;
}

bool ShortcutManager::Unbind(const KeyStroke& key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_bindings.erase(key) > 0;
}

bool ShortcutManager::Rebind(const KeyStroke& key, std::string newCommandId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_bindings.find(key);
    if (it == m_bindings.end()) return false;
    it->second = std::move(newCommandId);
    return true;
}

std::optional<std::string> ShortcutManager::Lookup(const KeyStroke& key) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_bindings.find(key);
    if (it == m_bindings.end()) return std::nullopt;
    return it->second;
}

bool ShortcutManager::HasConflict(const KeyStroke& key) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_bindings.count(key) > 0;
}

std::vector<std::pair<KeyStroke, std::string>> ShortcutManager::AllBindings() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::pair<KeyStroke, std::string>> result;
    result.reserve(m_bindings.size());
    for (const auto& [k, v] : m_bindings) {
        result.emplace_back(k, v);
    }
    return result;
}

size_t ShortcutManager::Count() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_bindings.size();
}

} // namespace AccessOS
