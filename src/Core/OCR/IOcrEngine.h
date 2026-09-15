#pragma once
// IOcrEngine.h — Abstract OCR engine interface (ACCESSOS-022)
//
// Implementations:
//   WinRtOcrEngine  — Windows.Media.Ocr (WinRT, Windows 10+) — EXPERIMENTAL
//   StubOcrEngine   — deterministic stub for unit tests

#include "OcrResult.h"
#include <string>
#include <cstdint>
#include <windows.h>  // HBITMAP, RECT

namespace AccessOS {
namespace OCR {

// Options passed to every recognition call
struct OcrOptions {
    std::string languageTag = "en-US";  // BCP-47 language tag
    float       minConfidence = 0.0f;   // filter words below this threshold (0 = keep all)
    bool        trimWhitespace = true;  // strip leading/trailing spaces from each word
};

class IOcrEngine {
public:
    virtual ~IOcrEngine() = default;

    // True if the engine is initialised and ready
    virtual bool IsAvailable() const = 0;

    // Recognise text in an HBITMAP (screen capture or GDI bitmap)
    // region: sub-region of the bitmap to analyse (empty = full bitmap)
    virtual OcrResult RecognizeFromBitmap(HBITMAP hBitmap,
                                          const OcrRect& region,
                                          const OcrOptions& opts) = 0;

    // Recognise text in an image file (PNG, BMP, JPEG)
    virtual OcrResult RecognizeFromFile(const std::string& filePath,
                                        const OcrOptions& opts) = 0;

    // Recognise text in a screen region captured live from the desktop
    // (takes a GDI screenshot of the given screen rect and feeds it to the engine)
    virtual OcrResult RecognizeScreenRegion(const OcrRect& screenRect,
                                            const OcrOptions& opts) = 0;
};

} // namespace OCR
} // namespace AccessOS
