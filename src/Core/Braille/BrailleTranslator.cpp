// BrailleTranslator.cpp — Grade 1 Braille translation (ACCESSOS-021)
#include "BrailleTranslator.h"

namespace AccessOS {
namespace Braille {

std::vector<BrailleCell> BrailleTranslator::Translate(const std::string& text) const {
    std::vector<BrailleCell> out;
    out.reserve(text.size() + 4);

    size_t i = 0;
    while (i < text.size()) {
        unsigned char ch = static_cast<unsigned char>(text[i]);

        // Skip multi-byte UTF-8 continuation bytes and high-byte starts
        if (ch >= 0x80u) {
            // Skip the full multi-byte sequence
            if (ch < 0xC0u) { ++i; continue; }      // continuation
            if (ch < 0xE0u) { i += 2; continue; }   // 2-byte start
            if (ch < 0xF0u) { i += 3; continue; }   // 3-byte start
            i += 4; continue;                         // 4-byte start
        }

        char c = static_cast<char>(ch);

        if (c == ' ' || c == '\t') {
            out.push_back(0x00); // empty cell for space/tab
            ++i;
        } else if (c >= 'A' && c <= 'Z') {
            EmitUppercase(out, c);
            ++i;
        } else if (c >= '0' && c <= '9') {
            EmitDigitRun(text, i, out);
        } else if (c >= 0x21 && c <= 0x7E) {
            // printable ASCII (punctuation / lowercase letters)
            BrailleCell cell = BrfToCell(c);
            if (cell != 0x00 || c == ' ') {
                out.push_back(cell);
            }
            ++i;
        } else {
            ++i; // skip control characters
        }
    }
    return out;
}

std::string BrailleTranslator::TranslateToUnicode(const std::string& text) const {
    return CellsToUtf8(Translate(text));
}

void BrailleTranslator::EmitUppercase(std::vector<BrailleCell>& out, char c) const {
    out.push_back(kCapitalIndicator);
    // Lowercase equivalent cell
    char lower = static_cast<char>(c - 'A' + 'a');
    out.push_back(BrfToCell(lower));
}

void BrailleTranslator::EmitDigitRun(const std::string& text, size_t& i,
                                      std::vector<BrailleCell>& out) const {
    out.push_back(kNumberIndicator);
    while (i < text.size()) {
        char c = text[i];
        if (c >= '0' && c <= '9') {
            out.push_back(BrfToCell(c));
            ++i;
        } else {
            break;
        }
    }
}

} // namespace Braille
} // namespace AccessOS
