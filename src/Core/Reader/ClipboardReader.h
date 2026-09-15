#pragma once
// ClipboardReader.h — Read clipboard text via Win32 (ACCESSOS-033)
//
// Reads CF_UNICODETEXT from the clipboard, converts UTF-16 → UTF-8,
// passes through PrivacyFilter, returns the sanitised string.

#include <string>

namespace AccessOS {

class ClipboardReader {
public:
    // Read current clipboard text. Returns empty string on failure or if empty.
    // The returned string is UTF-8.
    static std::string Read();

    // True if the clipboard currently contains text data.
    static bool HasText();
};

} // namespace AccessOS
