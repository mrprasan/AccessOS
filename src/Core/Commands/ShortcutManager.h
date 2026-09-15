// AccessOS/src/Core/Commands/ShortcutManager.h
//
// ShortcutManager — maps key combinations to command IDs.
//
// Why: Shortcuts must be user-customizable and centrally managed.
//      No component should hardcode key bindings outside this registry.
//      Conflict detection prevents two commands sharing the same shortcut.
//
// Threading: Thread-safe after initialization.

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <mutex>
#include <cstdint>

namespace AccessOS {

// Key modifiers as bit flags.
enum class KeyModifier : uint8_t {
    None    = 0,
    Shift   = 1 << 0,
    Ctrl    = 1 << 1,
    Alt     = 1 << 2,
    Win     = 1 << 3,
};

inline KeyModifier operator|(KeyModifier a, KeyModifier b) noexcept {
    return static_cast<KeyModifier>(
        static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
inline KeyModifier operator&(KeyModifier a, KeyModifier b) noexcept {
    return static_cast<KeyModifier>(
        static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}
inline bool HasModifier(KeyModifier mods, KeyModifier flag) noexcept {
    return (static_cast<uint8_t>(mods) & static_cast<uint8_t>(flag)) != 0;
}

// A key combination: virtual key code + modifier flags.
struct KeyStroke {
    uint32_t    vkCode    = 0;
    KeyModifier modifiers = KeyModifier::None;

    bool operator==(const KeyStroke& o) const noexcept {
        return vkCode == o.vkCode && modifiers == o.modifiers;
    }
};

struct KeyStrokeHash {
    size_t operator()(const KeyStroke& k) const noexcept {
        return std::hash<uint64_t>{}(
            (static_cast<uint64_t>(k.vkCode) << 8) |
             static_cast<uint64_t>(k.modifiers));
    }
};

class ShortcutManager {
public:
    ShortcutManager()  = default;
    ~ShortcutManager() = default;

    // Bind a KeyStroke to a command ID.
    // Returns false if the KeyStroke is already bound (conflict).
    bool Bind(KeyStroke key, std::string commandId);

    // Unbind a KeyStroke. Returns false if it was not bound.
    bool Unbind(const KeyStroke& key);

    // Rebind: replaces an existing binding. Returns false if key not found.
    bool Rebind(const KeyStroke& key, std::string newCommandId);

    // Returns the command ID for a given KeyStroke, or nullopt if none.
    std::optional<std::string> Lookup(const KeyStroke& key) const;

    // Returns true if the KeyStroke is already bound.
    bool HasConflict(const KeyStroke& key) const;

    // Returns all current bindings as (KeyStroke, commandId) pairs.
    std::vector<std::pair<KeyStroke, std::string>> AllBindings() const;

    // Returns the number of registered bindings.
    size_t Count() const;

private:
    mutable std::mutex                                              m_mutex;
    std::unordered_map<KeyStroke, std::string, KeyStrokeHash>      m_bindings;
};

} // namespace AccessOS
