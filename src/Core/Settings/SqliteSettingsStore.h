// AccessOS/src/Core/Settings/SqliteSettingsStore.h
//
// SQLite-backed implementation of ISettingsStore.
//
// Schema (single table):
//   CREATE TABLE settings (
//       key   TEXT PRIMARY KEY NOT NULL,
//       value TEXT NOT NULL
//   );
//
// Why SQLite: zero-dependency, single-file, ACID, well-tested on Windows.
//             The amalgamation is bundled — no system SQLite dependency.
//
// Threading: Protected by a std::mutex — all methods are thread-safe.
// File path: %APPDATA%\AccessOS\settings.db  (set by caller)

#pragma once
#include "ISettingsStore.h"

#include <string>
#include <mutex>

// Forward-declare sqlite3 so callers don't need to include sqlite3.h.
struct sqlite3;

namespace AccessOS {

class SqliteSettingsStore final : public ISettingsStore {
public:
    SqliteSettingsStore()  = default;
    ~SqliteSettingsStore() override;

    bool Open(const std::string& path)  override;
    void Close()                         override;
    bool IsOpen() const noexcept         override;

    bool                         Set(const std::string& key,
                                     const std::string& value) override;
    std::optional<std::string>   Get(const std::string& key) const override;
    std::string                  GetOr(const std::string& key,
                                       const std::string& defaultValue) const override;
    bool                         Remove(const std::string& key) override;
    bool                         Has(const std::string& key) const override;
    std::vector<std::string>     Keys(const std::string& prefix = "") const override;
    void                         Clear(const std::string& prefix = "") override;

private:
    // Execute a single SQL statement with no result set.
    bool Exec(const std::string& sql) const;

    mutable std::mutex  m_mutex;
    sqlite3*            m_db{ nullptr };
};

} // namespace AccessOS
