// OcrManager.cpp — coordinates OCR engine, caching, and integration (ACCESSOS-022)
#include "OcrManager.h"
#include <algorithm>

namespace AccessOS {
namespace OCR {

OcrManager::OcrManager() = default;

OcrManager::OcrManager(std::shared_ptr<IOcrEngine> engine)
    : m_engine(std::move(engine)) {}

void OcrManager::SetEngine(std::shared_ptr<IOcrEngine> engine) {
    std::unique_lock<std::shared_mutex> lk(m_mutex);
    m_engine    = std::move(engine);
    m_hasCached = false;
    m_lastResult = OcrResult{};
}

std::shared_ptr<IOcrEngine> OcrManager::GetEngine() const {
    std::shared_lock<std::shared_mutex> lk(m_mutex);
    return m_engine;
}

bool OcrManager::IsAvailable() const {
    std::shared_lock<std::shared_mutex> lk(m_mutex);
    return m_engine && m_engine->IsAvailable();
}

OcrResult OcrManager::RecognizeRegion(const OcrRect& screenRect,
                                       const OcrOptions& opts) {
    std::unique_lock<std::shared_mutex> lk(m_mutex);
    if (!m_engine) return {};
    OcrResult r = m_engine->RecognizeScreenRegion(screenRect, opts);
    r = ApplyFilters(std::move(r), opts);
    CacheResult(r);
    return r;
}

OcrResult OcrManager::RecognizeFromBitmap(HBITMAP hBitmap,
                                           const OcrRect& region,
                                           const OcrOptions& opts) {
    std::unique_lock<std::shared_mutex> lk(m_mutex);
    if (!m_engine) return {};
    OcrResult r = m_engine->RecognizeFromBitmap(hBitmap, region, opts);
    r = ApplyFilters(std::move(r), opts);
    CacheResult(r);
    return r;
}

OcrResult OcrManager::RecognizeFromFile(const std::string& filePath,
                                         const OcrOptions& opts) {
    std::unique_lock<std::shared_mutex> lk(m_mutex);
    if (!m_engine) return {};
    OcrResult r = m_engine->RecognizeFromFile(filePath, opts);
    r = ApplyFilters(std::move(r), opts);
    CacheResult(r);
    return r;
}

OcrResult OcrManager::GetLastResult() const {
    std::shared_lock<std::shared_mutex> lk(m_mutex);
    return m_lastResult;
}

void OcrManager::ClearCache() {
    std::unique_lock<std::shared_mutex> lk(m_mutex);
    m_hasCached  = false;
    m_lastResult = OcrResult{};
}

bool OcrManager::HasCachedResult() const {
    std::shared_lock<std::shared_mutex> lk(m_mutex);
    return m_hasCached;
}

// ── private ───────────────────────────────────────────────────────────────────

OcrResult OcrManager::ApplyFilters(OcrResult result,
                                    const OcrOptions& opts) const {
    if (opts.minConfidence <= 0.0f && !opts.trimWhitespace) return result;

    for (auto& line : result.lines) {
        // Filter words by confidence
        if (opts.minConfidence > 0.0f) {
            line.words.erase(
                std::remove_if(line.words.begin(), line.words.end(),
                    [&](const OcrWord& w) {
                        return w.confidence < opts.minConfidence;
                    }),
                line.words.end());
        }
        // Trim whitespace
        if (opts.trimWhitespace) {
            for (auto& w : line.words) {
                // ltrim
                size_t start = w.text.find_first_not_of(" \t\r\n");
                if (start == std::string::npos) { w.text.clear(); continue; }
                // rtrim
                size_t end = w.text.find_last_not_of(" \t\r\n");
                w.text = w.text.substr(start, end - start + 1);
            }
        }
        // Remove words that became empty after trimming
        line.words.erase(
            std::remove_if(line.words.begin(), line.words.end(),
                [](const OcrWord& w) { return w.text.empty(); }),
            line.words.end());
    }
    // Remove lines that became empty
    result.lines.erase(
        std::remove_if(result.lines.begin(), result.lines.end(),
            [](const OcrLine& l) { return l.words.empty(); }),
        result.lines.end());

    return result;
}

void OcrManager::CacheResult(const OcrResult& result) {
    m_lastResult = result;
    m_hasCached  = result.succeeded;
}

} // namespace OCR
} // namespace AccessOS
