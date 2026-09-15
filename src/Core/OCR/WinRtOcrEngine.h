#pragma once
// WinRtOcrEngine.h — Windows.Media.Ocr implementation (ACCESSOS-022)
//
// Status: EXPERIMENTAL
// Uses the WinRT Windows.Media.Ocr.OcrEngine API available on Windows 10+.
// Requires the app to be packaged OR have identity to use WinRT APIs.
// For unpackaged processes, falls back to IsAvailable()=false.
//
// The implementation captures a GDI screenshot, converts it to a
// Windows.Graphics.Imaging.SoftwareBitmap, then calls OcrEngine::RecognizeAsync.
// All WinRT calls are dispatched synchronously via a blocking co_await on a
// background thread to avoid UI-thread deadlocks.

#include "IOcrEngine.h"
#include <string>

namespace AccessOS {
namespace OCR {

class WinRtOcrEngine : public IOcrEngine {
public:
    WinRtOcrEngine();
    ~WinRtOcrEngine() override;

    // Returns true if Windows.Media.Ocr is available on this system.
    // Will be false on Windows 8.1 or when WinRT activation fails.
    bool IsAvailable() const override;

    OcrResult RecognizeFromBitmap(HBITMAP hBitmap,
                                  const OcrRect& region,
                                  const OcrOptions& opts) override;

    OcrResult RecognizeFromFile(const std::string& filePath,
                                const OcrOptions& opts) override;

    OcrResult RecognizeScreenRegion(const OcrRect& screenRect,
                                    const OcrOptions& opts) override;

private:
    bool m_available;

    // Take a GDI screenshot of screenRect; returns owned HBITMAP (caller must DeleteObject)
    HBITMAP CaptureScreenRect(const OcrRect& screenRect);

    // Convert HBITMAP region to OcrResult via WinRT OCR.
    // Returns failed result if WinRT is unavailable.
    OcrResult RunWinRtOcr(HBITMAP hBitmap, const OcrRect& region,
                           const OcrOptions& opts);
};

} // namespace OCR
} // namespace AccessOS
