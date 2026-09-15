// AccessOS/src/Core/Settings/ISettingsStore.h
//
// ISettingsStore — abstract key-value persistence interface.
//
// Why: Settings storage must be swappable (SQLite on disk, in-memory for tests,
//      registry for future OS integration). All callers depend on this interface,
//      not on a specific backend.
//
// Key convention:  "section.key"  — e.g. "speech.rate", "shortcuts.nav.next"
// Value type:      string (UTF-8). Typed getters live in SettingsManager.
//
// Threading: Implementations must be thread-safe.

#pragma once
#include <string>
#include <optional>
#include <vector>

namespace AccessOS {

class ISettingsStore {
public:
    virtual ~ISettingsStore() = default;

    // Open / close the backing store.
    // path is implementation-defined (file path, ":memory:", registry root, …).
    virtual bool Open(const std::string& path) = 0;
    virtual void Close() = 0;
    virtual bool IsOpen() const noexcept = 0;

    // Write a value. Creates the key if it does not exist.
    virtual bool Set(const std::string& key, const std::string& value) = 0;

    // Read a value. Returns nullopt if the key does not exist.
    virtual std::optional<std::string> Get(const std::string& key) const = 0;

    // Read a value, returning defaultValue if the key does not exist.
    virtual std::string GetOr(const std::string& key,
                               const std::string& defaultValue) const = 0;

    // Delete a key. Returns false if the key did not exist.
    virtual bool Remove(const std::string& key) = 0;

    // Returns true if the key exists.
    virtual bool Has(const std::string& key) const = 0;

    // Returns all keys whose prefix matches the given section (e.g. "speech.").
    virtual std::vector<std::string> Keys(const std::string& prefix = "") const = 0;

    // Delete all keys under a given prefix.
    virtual void Clear(const std::string& prefix = "") = 0;
};

} // namespace AccessOS
