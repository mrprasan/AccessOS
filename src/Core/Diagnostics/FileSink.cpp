// AccessOS/src/Core/Diagnostics/FileSink.cpp

#include "FileSink.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace AccessOS {

bool FileSink::Open(const std::string& path) {
    std::unique_lock<std::mutex> lk(m_mutex);
    if (m_open) m_file.close();
    m_file.open(path, std::ios::app | std::ios::out);
    m_open = m_file.is_open();
    return m_open;
}

void FileSink::Close() {
    std::unique_lock<std::mutex> lk(m_mutex);
    if (m_open) {
        m_file.flush();
        m_file.close();
        m_open = false;
    }
}

bool FileSink::IsOpen() const noexcept {
    std::unique_lock<std::mutex> lk(m_mutex);
    return m_open;
}

void FileSink::Write(const LogEntry& entry) {
    std::unique_lock<std::mutex> lk(m_mutex);
    if (!m_open) return;

    // Build timestamp HH:MM:SS.mmm using system_clock.
    const auto now   = std::chrono::system_clock::now();
    const auto ms    = std::chrono::duration_cast<std::chrono::milliseconds>(
                           now.time_since_epoch()) % 1000;
    const std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm tmBuf{};
#ifdef _WIN32
    localtime_s(&tmBuf, &tt);
#else
    localtime_r(&tt, &tmBuf);
#endif

    static const char* kLevels[] = { "DEBUG", "INFO ", "WARN ", "ERROR", "CRIT " };
    const char* lvl = (static_cast<int>(entry.level) < 5)
                      ? kLevels[static_cast<int>(entry.level)]
                      : "?????";

    m_file << '['
           << std::setfill('0') << std::setw(2) << tmBuf.tm_hour << ':'
           << std::setw(2) << tmBuf.tm_min  << ':'
           << std::setw(2) << tmBuf.tm_sec  << '.'
           << std::setw(3) << ms.count()    << ']'
           << '[' << lvl << ']'
           << '[' << entry.component << "] "
           << entry.message << '\n';

    m_file.flush();
}

} // namespace AccessOS
