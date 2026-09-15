// ClipboardReader.cpp — Read clipboard text via Win32 (ACCESSOS-033)
#include "ClipboardReader.h"
#include <windows.h>
#include <string>

namespace AccessOS {

bool ClipboardReader::HasText() {
    return ::IsClipboardFormatAvailable(CF_UNICODETEXT) != 0;
}

std::string ClipboardReader::Read() {
    if (!HasText()) return {};

    if (!::OpenClipboard(nullptr)) return {};

    std::string result;
    HANDLE hData = ::GetClipboardData(CF_UNICODETEXT);
    if (hData) {
        const wchar_t* pWide = static_cast<const wchar_t*>(::GlobalLock(hData));
        if (pWide) {
            // Convert UTF-16LE → UTF-8
            int len = ::WideCharToMultiByte(CP_UTF8, 0,
                                            pWide, -1,
                                            nullptr, 0,
                                            nullptr, nullptr);
            if (len > 1) { // len includes null terminator
                result.resize(static_cast<size_t>(len - 1));
                ::WideCharToMultiByte(CP_UTF8, 0,
                                      pWide, -1,
                                      result.data(), len,
                                      nullptr, nullptr);
            }
            ::GlobalUnlock(hData);
        }
    }
    ::CloseClipboard();
    return result;
}

} // namespace AccessOS
