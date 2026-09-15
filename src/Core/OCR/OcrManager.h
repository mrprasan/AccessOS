#pragma once
// OcrManager.h — coordinates OCR engine, caching, and AccessNode integration (ACCESSOS-022)
//
// Responsibilities:
//   - Owns one IOcrEngine instance
//   - Caches the last OcrResult (invalidated on new recognition)
//   - Provides synchronous RecognizeRegion / RecognizeNode entry points
//   - Filters words by confidence threshold from OcrOptions
//   - Thread-safe: uses shared_mutex (read-heavy: cache hits)

#include "IOcrEngine.h"
#include <memory>
#include <shared_mutex>
#include <string>

namespace AccessOS {
namespace OCR {

class OcrManager {
public:
    OcrManager();
    explicit OcrManager(std::shared_ptr<IOcrEngine> engine);

    // Replace the active engine (resets cache)
    void SetEngine(std::shared_ptr<IOcrEngine> engine);
    std::shared_ptr<IOcrEngine> GetEngine() const;

    bool IsAvailable() const;

    // Recognise a screen region; caches result
    OcrResult RecognizeRegion(const OcrRect& screenRect,
                              const OcrOptions& opts = {});

    // Recognise a bitmap; caches result
    OcrResult RecognizeFromBitmap(HBITMAP hBitmap,
                                  const OcrRect& region,
                                  const OcrOptions& opts = {});

    // Recognise from file; caches result
    OcrResult RecognizeFromFile(const std::string& filePath,
                                const OcrOptions& opts = {});

    // Return the last cached result (empty/failed if none)
    OcrResult GetLastResult() const;

    // Clear the cached result
    void ClearCache();

    // True if a cached result is available
    bool HasCachedResult() const;

private:
    std::shared_ptr<IOcrEngine>  m_engine;
    OcrResult                    m_lastResult;
    bool                         m_hasCached = false;
    mutable std::shared_mutex    m_mutex;

    OcrResult ApplyFilters(OcrResult result, const OcrOptions& opts) const;
    void CacheResult(const OcrResult& result);
};

} // namespace OCR
} // namespace AccessOS
