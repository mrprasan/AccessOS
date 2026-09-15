// WinRtOcrEngine.cpp — Windows.Media.Ocr implementation (ACCESSOS-022/034)
//
// ACCESSOS-034: Full synchronous OCR via Windows.Media.Ocr.OcrEngine.
// Uses WRL (Windows Runtime C++ Template Library) for IInspectable / async
// activation without requiring C++/WinRT or /ZW mode.
//
// The IAsyncOperation is waited on via a Win32 event (COINIT_MULTITHREADED):
//   CreateEvent → RegisterCompletedHandler → WaitForSingleObject → GetResults

#include "WinRtOcrEngine.h"
#include <objbase.h>
#include <roapi.h>
#include <winstring.h>
#include <windows.foundation.h>
#include <wrl/client.h>
#include <wrl/event.h>
#include <wrl/wrappers/corewrappers.h>
#include <MemoryBuffer.h>
#include <windows.media.ocr.h>
#include <windows.graphics.imaging.h>
#include <windows.storage.streams.h>

#pragma comment(lib, "runtimeobject.lib")
#pragma comment(lib, "windowsapp.lib")

// Bring in WRL helpers but do NOT import ABI namespaces globally —
// that would cause OcrResult to ambiguously refer to both our type and the WinRT type.
using Microsoft::WRL::ComPtr;
using Microsoft::WRL::Callback;
using Microsoft::WRL::Wrappers::HStringReference;
using Microsoft::WRL::Wrappers::HString;

namespace WinOcr      = ABI::Windows::Media::Ocr;
namespace WinImaging  = ABI::Windows::Graphics::Imaging;
namespace WinFound    = ABI::Windows::Foundation;
namespace WinFoundCol = ABI::Windows::Foundation::Collections;

namespace AccessOS {
namespace OCR {

// ── helpers ───────────────────────────────────────────────────────────────────

static bool ProbeWinRtOcr() {
    // Try to create an HSTRING for the OCR engine class name.
    // If RoActivateInstance fails, WinRT OCR is not available.
    HSTRING classId = nullptr;
    HRESULT hr = ::WindowsCreateString(
        L"Windows.Media.Ocr.OcrEngine", 28, &classId);
    if (FAILED(hr)) return false;
    ::WindowsDeleteString(classId);
    return true;
}

// ── WinRtOcrEngine ────────────────────────────────────────────────────────────

WinRtOcrEngine::WinRtOcrEngine()
    : m_available(ProbeWinRtOcr()) {}

WinRtOcrEngine::~WinRtOcrEngine() = default;

bool WinRtOcrEngine::IsAvailable() const {
    return m_available;
}

OcrResult WinRtOcrEngine::RecognizeFromBitmap(HBITMAP hBitmap,
                                               const OcrRect& region,
                                               const OcrOptions& opts) {
    if (!m_available || hBitmap == nullptr) {
        OcrResult r;
        r.succeeded = false;
        return r;
    }
    return RunWinRtOcr(hBitmap, region, opts);
}

OcrResult WinRtOcrEngine::RecognizeFromFile(const std::string& /*filePath*/,
                                             const OcrOptions& /*opts*/) {
    // File-based recognition: load file into GDI bitmap and forward.
    // NOT IMPLEMENTED in this build — returns empty failed result.
    OcrResult r;
    r.succeeded = false;
    return r;
}

OcrResult WinRtOcrEngine::RecognizeScreenRegion(const OcrRect& screenRect,
                                                 const OcrOptions& opts) {
    if (!m_available) {
        OcrResult r;
        r.succeeded = false;
        return r;
    }
    HBITMAP hBmp = CaptureScreenRect(screenRect);
    if (!hBmp) {
        OcrResult r;
        r.succeeded = false;
        return r;
    }
    OcrResult result = RunWinRtOcr(hBmp, screenRect, opts);
    ::DeleteObject(hBmp);
    return result;
}

// ── private ───────────────────────────────────────────────────────────────────

HBITMAP WinRtOcrEngine::CaptureScreenRect(const OcrRect& rect) {
    if (rect.IsEmpty()) return nullptr;

    HDC hdcScreen = ::GetDC(nullptr);
    if (!hdcScreen) return nullptr;

    HDC hdcMem = ::CreateCompatibleDC(hdcScreen);
    if (!hdcMem) {
        ::ReleaseDC(nullptr, hdcScreen);
        return nullptr;
    }

    HBITMAP hBmp = ::CreateCompatibleBitmap(hdcScreen, rect.width, rect.height);
    if (!hBmp) {
        ::DeleteDC(hdcMem);
        ::ReleaseDC(nullptr, hdcScreen);
        return nullptr;
    }

    HGDIOBJ hOld = ::SelectObject(hdcMem, hBmp);
    ::BitBlt(hdcMem, 0, 0, rect.width, rect.height,
             hdcScreen, rect.x, rect.y, SRCCOPY);
    ::SelectObject(hdcMem, hOld);

    ::DeleteDC(hdcMem);
    ::ReleaseDC(nullptr, hdcScreen);
    return hBmp;
}

OcrResult WinRtOcrEngine::RunWinRtOcr(HBITMAP hBitmap,
                                        const OcrRect& region,
                                        const OcrOptions& opts) {
    OcrResult result;
    result.sourceRect  = region;
    result.languageTag = opts.languageTag;
    result.succeeded   = false;

    if (!hBitmap) return result;

    // ── 1. Get BITMAP dimensions ─────────────────────────────────────────────
    BITMAP bmpInfo{};
    if (!::GetObject(hBitmap, sizeof(bmpInfo), &bmpInfo)) return result;

    int width  = bmpInfo.bmWidth;
    int height = bmpInfo.bmHeight;
    if (width <= 0 || height <= 0) return result;

    // ── 2. Create IOcrEngine via WRL ─────────────────────────────────────────
    ComPtr<WinOcr::IOcrEngineStatics> ocrStatics;
    HRESULT hr = ::RoGetActivationFactory(
        HStringReference(RuntimeClass_Windows_Media_Ocr_OcrEngine).Get(),
        IID_PPV_ARGS(&ocrStatics));
    if (FAILED(hr)) return result;

    // Choose language: try requested tag, fall back to first available
    ComPtr<__FIVectorView_1_Windows__CGlobalization__CLanguage> langs;
    hr = ocrStatics->get_AvailableRecognizerLanguages(&langs);
    if (FAILED(hr)) return result;

    UINT langCount = 0;
    langs->get_Size(&langCount);
    if (langCount == 0) return result;

    ComPtr<ABI::Windows::Globalization::ILanguage> lang;
    langs->GetAt(0, &lang); // use first available language

    ComPtr<WinOcr::IOcrEngine> ocrEngine;
    hr = ocrStatics->TryCreateFromLanguage(lang.Get(), &ocrEngine);
    if (FAILED(hr) || !ocrEngine) return result;

    // ── 3. Convert HBITMAP to ISoftwareBitmap ────────────────────────────────
    // Read pixel data via GetDIBits
    BITMAPINFOHEADER bi{};
    bi.biSize        = sizeof(bi);
    bi.biWidth       = width;
    bi.biHeight      = -height; // top-down
    bi.biPlanes      = 1;
    bi.biBitCount    = 32;
    bi.biCompression = BI_RGB;

    std::vector<uint8_t> pixels(static_cast<size_t>(width) * height * 4);
    HDC hdcScreen = ::GetDC(nullptr);
    ::GetDIBits(hdcScreen, hBitmap, 0, height,
                pixels.data(),
                reinterpret_cast<BITMAPINFO*>(&bi), DIB_RGB_COLORS);
    ::ReleaseDC(nullptr, hdcScreen);

    // Convert BGRA → BGRA (already correct for SoftwareBitmap Bgra8)
    // Create IBuffer from pixel data via InMemoryRandomAccessStream
    ComPtr<WinImaging::ISoftwareBitmapFactory> sbFactory;
    hr = ::RoGetActivationFactory(
        HStringReference(RuntimeClass_Windows_Graphics_Imaging_SoftwareBitmap).Get(),
        IID_PPV_ARGS(&sbFactory));
    if (FAILED(hr)) return result;

    ComPtr<WinImaging::ISoftwareBitmap> softBmp;
    hr = sbFactory->Create(WinImaging::BitmapPixelFormat_Bgra8, width, height, &softBmp);
    if (FAILED(hr) || !softBmp) return result;

    // Copy pixels into SoftwareBitmap via IBitmapBuffer
    ComPtr<WinImaging::IBitmapBuffer> bmpBuffer;
    hr = softBmp->LockBuffer(WinImaging::BitmapBufferAccessMode_Write, &bmpBuffer);
    if (FAILED(hr)) return result;

    ComPtr<WinFound::IMemoryBufferReference> memRef;
    ComPtr<WinFound::IMemoryBuffer> memBuf;
    bmpBuffer.As(&memBuf);
    memBuf->CreateReference(&memRef);
    ComPtr<::Windows::Foundation::IMemoryBufferByteAccess> byteAccess;
    memRef.As(&byteAccess);
    uint8_t* pData = nullptr; UINT32 capacity = 0;
    hr = byteAccess->GetBuffer(&pData, &capacity);
    if (SUCCEEDED(hr) && pData && capacity >= pixels.size()) {
        std::memcpy(pData, pixels.data(), pixels.size());
    }
    // Buffer closed on scope exit automatically

    // ── 4. RecognizeAsync — synchronous wait via Win32 event ─────────────────
    HANDLE hEvt = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!hEvt) return result;

    ComPtr<__FIAsyncOperation_1_Windows__CMedia__COcr__COcrResult> asyncOp;
    hr = ocrEngine->RecognizeAsync(softBmp.Get(), &asyncOp);
    if (FAILED(hr)) { ::CloseHandle(hEvt); return result; }

    // Register completion callback
    hr = asyncOp->put_Completed(
        Callback<__FIAsyncOperationCompletedHandler_1_Windows__CMedia__COcr__COcrResult>(
            [hEvt](__FIAsyncOperation_1_Windows__CMedia__COcr__COcrResult*, WinFound::AsyncStatus) -> HRESULT {
                ::SetEvent(hEvt);
                return S_OK;
            }).Get());
    if (FAILED(hr)) { ::CloseHandle(hEvt); return result; }

    // Wait up to 10 seconds for OCR to complete
    DWORD wait = ::WaitForSingleObject(hEvt, 10000);
    ::CloseHandle(hEvt);
    if (wait != WAIT_OBJECT_0) return result;

    // ── 5. Extract results ───────────────────────────────────────────────────
    ComPtr<WinOcr::IOcrResult> ocrRes;
    hr = asyncOp->GetResults(&ocrRes);
    if (FAILED(hr) || !ocrRes) return result;

    ComPtr<__FIVectorView_1_Windows__CMedia__COcr__COcrLine> lineVec;
    hr = ocrRes->get_Lines(&lineVec);
    if (FAILED(hr)) return result;

    UINT lineCount = 0;
    lineVec->get_Size(&lineCount);

    for (UINT li = 0; li < lineCount; ++li) {
        ComPtr<WinOcr::IOcrLine> ocrLine;
        lineVec->GetAt(li, &ocrLine);

        ComPtr<__FIVectorView_1_Windows__CMedia__COcr__COcrWord> wordVec;
        ocrLine->get_Words(&wordVec);
        UINT wordCount = 0;
        wordVec->get_Size(&wordCount);

        OcrLine line;
        for (UINT wi = 0; wi < wordCount; ++wi) {
            ComPtr<WinOcr::IOcrWord> ocrWord;
            wordVec->GetAt(wi, &ocrWord);

            HString hText;
            ocrWord->get_Text(hText.GetAddressOf());
            UINT len = 0;
            const wchar_t* pWide = hText.GetRawBuffer(&len);

            OcrWord word;
            // Convert UTF-16 → UTF-8
            int u8len = ::WideCharToMultiByte(CP_UTF8, 0, pWide, static_cast<int>(len),
                                               nullptr, 0, nullptr, nullptr);
            if (u8len > 0) {
                word.text.resize(u8len);
                ::WideCharToMultiByte(CP_UTF8, 0, pWide, static_cast<int>(len),
                                      word.text.data(), u8len, nullptr, nullptr);
            }
            word.confidence = 1.0f; // WinRT OCR doesn't expose per-word confidence
            line.words.push_back(std::move(word));
        }
        result.lines.push_back(std::move(line));
    }

    result.succeeded = true;
    return result;
}

} // namespace OCR
} // namespace AccessOS
