// AccessOS/src/Core/Settings/SqliteSettingsStore.cpp

#include "SqliteSettingsStore.h"
#include "../Logging/Logger.h"

#include <sqlite3.h>

#include <cstring>

namespace AccessOS {

static constexpr const char* kComp = "SqliteSettingsStore";

// ── Destructor ────────────────────────────────────────────────────────────────

SqliteSettingsStore::~SqliteSettingsStore() {
    Close();
}

// ── Open / Close ──────────────────────────────────────────────────────────────

bool SqliteSettingsStore::Open(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_db) return true;  // Already open.

    int rc = sqlite3_open(path.c_str(), &m_db);
    if (rc != SQLITE_OK) {
        ACOS_LOG_ERROR(kComp, "sqlite3_open failed: " +
            std::string(sqlite3_errmsg(m_db)));
        sqlite3_close(m_db);
        m_db = nullptr;
        return false;
    }

    // Enable WAL mode for better concurrent read performance.
    sqlite3_exec(m_db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);

    // Create the settings table if it doesn't exist.
    static const char* kCreate =
        "CREATE TABLE IF NOT EXISTS settings ("
        "  key   TEXT PRIMARY KEY NOT NULL,"
        "  value TEXT NOT NULL"
        ");";

    char* errMsg = nullptr;
    rc = sqlite3_exec(m_db, kCreate, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        ACOS_LOG_ERROR(kComp, "CREATE TABLE failed: " +
            std::string(errMsg ? errMsg : "unknown"));
        sqlite3_free(errMsg);
        sqlite3_close(m_db);
        m_db = nullptr;
        return false;
    }

    ACOS_LOG_INFO(kComp, "Opened settings DB: " + path);
    return true;
}

void SqliteSettingsStore::Close() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_db) return;
    sqlite3_close(m_db);
    m_db = nullptr;
    ACOS_LOG_INFO(kComp, "Settings DB closed");
}

bool SqliteSettingsStore::IsOpen() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_db != nullptr;
}

// ── Set ───────────────────────────────────────────────────────────────────────

bool SqliteSettingsStore::Set(const std::string& key,
                               const std::string& value)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_db) return false;

    static const char* kSQL =
        "INSERT INTO settings(key, value) VALUES(?1, ?2)"
        "  ON CONFLICT(key) DO UPDATE SET value=excluded.value;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, kSQL, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, key.c_str(),   -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, value.c_str(), -1, SQLITE_STATIC);

    const bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

// ── Get ───────────────────────────────────────────────────────────────────────

std::optional<std::string> SqliteSettingsStore::Get(
    const std::string& key) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_db) return std::nullopt;

    static const char* kSQL =
        "SELECT value FROM settings WHERE key = ?1 LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, kSQL, -1, &stmt, nullptr) != SQLITE_OK)
        return std::nullopt;

    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_STATIC);

    std::optional<std::string> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* text = reinterpret_cast<const char*>(
            sqlite3_column_text(stmt, 0));
        if (text) result = std::string(text);
    }
    sqlite3_finalize(stmt);
    return result;
}

std::string SqliteSettingsStore::GetOr(const std::string& key,
                                        const std::string& defaultValue) const
{
    auto v = Get(key);
    return v.has_value() ? *v : defaultValue;
}

// ── Remove ────────────────────────────────────────────────────────────────────

bool SqliteSettingsStore::Remove(const std::string& key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_db) return false;

    static const char* kSQL =
        "DELETE FROM settings WHERE key = ?1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, kSQL, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    const int changes = sqlite3_changes(m_db);
    sqlite3_finalize(stmt);
    return changes > 0;
}

// ── Has ───────────────────────────────────────────────────────────────────────

bool SqliteSettingsStore::Has(const std::string& key) const {
    return Get(key).has_value();
}

// ── Keys ──────────────────────────────────────────────────────────────────────

std::vector<std::string> SqliteSettingsStore::Keys(
    const std::string& prefix) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_db) return {};

    std::vector<std::string> result;

    if (prefix.empty()) {
        static const char* kSQL = "SELECT key FROM settings ORDER BY key;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(m_db, kSQL, -1, &stmt, nullptr) != SQLITE_OK)
            return result;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* k = reinterpret_cast<const char*>(
                sqlite3_column_text(stmt, 0));
            if (k) result.emplace_back(k);
        }
        sqlite3_finalize(stmt);
    } else {
        // Match keys that start with prefix.
        // Use LIKE with '%' wildcard — escape any literal '%' or '_' in prefix.
        std::string pattern;
        pattern.reserve(prefix.size() + 2);
        for (char c : prefix) {
            if (c == '%' || c == '_' || c == '\\') pattern += '\\';
            pattern += c;
        }
        pattern += '%';

        static const char* kSQL =
            "SELECT key FROM settings WHERE key LIKE ?1 ESCAPE '\\' ORDER BY key;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(m_db, kSQL, -1, &stmt, nullptr) != SQLITE_OK)
            return result;
        sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* k = reinterpret_cast<const char*>(
                sqlite3_column_text(stmt, 0));
            if (k) result.emplace_back(k);
        }
        sqlite3_finalize(stmt);
    }
    return result;
}

// ── Clear ─────────────────────────────────────────────────────────────────────

void SqliteSettingsStore::Clear(const std::string& prefix) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_db) return;

    if (prefix.empty()) {
        sqlite3_exec(m_db, "DELETE FROM settings;", nullptr, nullptr, nullptr);
        return;
    }

    std::string pattern;
    pattern.reserve(prefix.size() + 2);
    for (char c : prefix) {
        if (c == '%' || c == '_' || c == '\\') pattern += '\\';
        pattern += c;
    }
    pattern += '%';

    static const char* kSQL =
        "DELETE FROM settings WHERE key LIKE ?1 ESCAPE '\\';";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, kSQL, -1, &stmt, nullptr) != SQLITE_OK)
        return;
    sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

// ── Private helper ────────────────────────────────────────────────────────────

bool SqliteSettingsStore::Exec(const std::string& sql) const {
    if (!m_db) return false;
    char* errMsg = nullptr;
    const int rc = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        ACOS_LOG_ERROR(kComp, "SQL error: " +
            std::string(errMsg ? errMsg : "unknown"));
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

} // namespace AccessOS
