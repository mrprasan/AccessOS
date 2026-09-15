#pragma once
// BrailleCells.h — 8-dot Braille cell encoding (ACCESSOS-021)
//
// Dot-bit convention: dot1=bit0, dot2=bit1, dot3=bit2, dot4=bit3,
//                     dot5=bit4, dot6=bit5, dot7=bit6, dot8=bit7
// Unicode Braille block: U+2800 + cell_bits
// BRF/NABCC: ASCII 0x20-0x7E mapped to 6-dot (bits 0-5) Braille patterns.

#include <cstdint>
#include <string>
#include <vector>
#include <optional>

namespace AccessOS {
namespace Braille {

// 8-dot Braille cell: bits 0-7 represent dots 1-8
using BrailleCell = uint8_t;

// ── Unicode helpers ───────────────────────────────────────────────────────────

inline char32_t CellToUnicode(BrailleCell cell) noexcept {
    return static_cast<char32_t>(0x2800u + cell);
}

inline std::optional<BrailleCell> UnicodeToCellBits(char32_t cp) noexcept {
    if (cp < 0x2800u || cp > 0x28FFu) return std::nullopt;
    return static_cast<BrailleCell>(cp - 0x2800u);
}

// ── Dot accessors (1-based) ───────────────────────────────────────────────────

inline bool HasDot(BrailleCell cell, int dot) noexcept {
    if (dot < 1 || dot > 8) return false;
    return (cell >> (dot - 1)) & 1u;
}

inline BrailleCell SetDot(BrailleCell cell, int dot) noexcept {
    if (dot < 1 || dot > 8) return cell;
    return cell | static_cast<uint8_t>(1u << (dot - 1));
}

inline BrailleCell ClearDot(BrailleCell cell, int dot) noexcept {
    if (dot < 1 || dot > 8) return cell;
    return cell & ~static_cast<uint8_t>(1u << (dot - 1));
}

// ── UTF-8 encoding helper ─────────────────────────────────────────────────────

inline void AppendUtf8(std::string& out, char32_t cp) {
    if (cp < 0x80u) {
        out += static_cast<char>(cp);
    } else if (cp < 0x800u) {
        out += static_cast<char>(0xC0u | (cp >> 6));
        out += static_cast<char>(0x80u | (cp & 0x3Fu));
    } else if (cp < 0x10000u) {
        out += static_cast<char>(0xE0u | (cp >> 12));
        out += static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu));
        out += static_cast<char>(0x80u | (cp & 0x3Fu));
    } else {
        out += static_cast<char>(0xF0u | (cp >> 18));
        out += static_cast<char>(0x80u | ((cp >> 12) & 0x3Fu));
        out += static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu));
        out += static_cast<char>(0x80u | (cp & 0x3Fu));
    }
}

// ── BRF/NABCC → BrailleCell ───────────────────────────────────────────────────
// North American Braille Computer Code (NABCC): printable ASCII 0x20-0x7E
// maps to 6-dot Braille patterns (bits 0-5).
// dot1=lsb (bit0), dot6=bit5.

inline BrailleCell BrfToCell(char c) noexcept {
    // Authoritative NABCC 6-dot table, indexed by (c - 0x20), range [0..94]
    static constexpr uint8_t kFull[95] = {
        /* ' '  0x20 */ 0x00,
        /* '!'  0x21 */ 0x16, // dots 2,3,5   = b1+b2+b4
        /* '"'  0x22 */ 0x04, // dot 3        = b2
        /* '#'  0x23 */ 0x3C, // dots 3,4,5,6 = b2+b3+b4+b5
        /* '$'  0x24 */ 0x12, // dots 2,5     = b1+b4
        /* '%'  0x25 */ 0x31, // dots 1,5,6   = b0+b4+b5
        /* '&'  0x26 */ 0x2A, // dots 2,4,6   = b1+b3+b5
        /* '\'' 0x27 */ 0x02, // dot 2        = b1
        /* '('  0x28 */ 0x36, // dots 2,3,5,6 = b1+b2+b4+b5
        /* ')'  0x29 */ 0x2E, // dots 2,3,4,6 = b1+b2+b3+b5
        /* '*'  0x2A */ 0x14, // dots 3,5     = b2+b4
        /* '+'  0x2B */ 0x30, // dots 5,6     = b4+b5
        /* ','  0x2C */ 0x04, // dot 3        = b2
        /* '-'  0x2D */ 0x24, // dots 3,6     = b2+b5
        /* '.'  0x2E */ 0x32, // dots 2,5,6   = b1+b4+b5
        /* '/'  0x2F */ 0x0C, // dots 3,4     = b2+b3
        /* '0'  0x30 */ 0x3A, // dots 2,4,5,6 = b1+b3+b4+b5
        /* '1'  0x31 */ 0x02, // dot 2
        /* '2'  0x32 */ 0x06, // dots 2,3     = b1+b2
        /* '3'  0x33 */ 0x12, // dots 2,5     = b1+b4
        /* '4'  0x34 */ 0x32, // dots 2,5,6   = b1+b4+b5
        /* '5'  0x35 */ 0x22, // dots 2,6     = b1+b5
        /* '6'  0x36 */ 0x16, // dots 2,3,5   = b1+b2+b4
        /* '7'  0x37 */ 0x36, // dots 2,3,5,6 = b1+b2+b4+b5
        /* '8'  0x38 */ 0x26, // dots 2,3,6   = b1+b2+b5
        /* '9'  0x39 */ 0x12, // dots 2,5     = b1+b4
        /* ':'  0x3A */ 0x06, // dots 2,3
        /* ';'  0x3B */ 0x08, // dot 4        = b3
        /* '<'  0x3C */ 0x20, // dot 6        = b5
        /* '='  0x3D */ 0x18, // dots 4,5     = b3+b4
        /* '>'  0x3E */ 0x10, // dot 5        = b4
        /* '?'  0x3F */ 0x31, // dots 1,5,6   = b0+b4+b5
        /* '@'  0x40 */ 0x3F, // dots 1-6     = 0x3F
        /* 'A'  0x41 */ 0x01, // dot 1
        /* 'B'  0x42 */ 0x03, // dots 1,2
        /* 'C'  0x43 */ 0x09, // dots 1,4
        /* 'D'  0x44 */ 0x19, // dots 1,4,5
        /* 'E'  0x45 */ 0x11, // dots 1,5
        /* 'F'  0x46 */ 0x0B, // dots 1,2,4
        /* 'G'  0x47 */ 0x1B, // dots 1,2,4,5
        /* 'H'  0x48 */ 0x13, // dots 1,2,5
        /* 'I'  0x49 */ 0x0A, // dots 2,4
        /* 'J'  0x4A */ 0x1A, // dots 2,4,5
        /* 'K'  0x4B */ 0x05, // dots 1,3
        /* 'L'  0x4C */ 0x07, // dots 1,2,3
        /* 'M'  0x4D */ 0x0D, // dots 1,3,4
        /* 'N'  0x4E */ 0x1D, // dots 1,3,4,5
        /* 'O'  0x4F */ 0x15, // dots 1,3,5
        /* 'P'  0x50 */ 0x0F, // dots 1,2,3,4
        /* 'Q'  0x51 */ 0x1F, // dots 1,2,3,4,5
        /* 'R'  0x52 */ 0x17, // dots 1,2,3,5
        /* 'S'  0x53 */ 0x0E, // dots 2,3,4
        /* 'T'  0x54 */ 0x1E, // dots 2,3,4,5
        /* 'U'  0x55 */ 0x25, // dots 1,3,6
        /* 'V'  0x56 */ 0x27, // dots 1,2,3,6
        /* 'W'  0x57 */ 0x3A, // dots 2,4,5,6
        /* 'X'  0x58 */ 0x2D, // dots 1,3,4,6
        /* 'Y'  0x59 */ 0x3D, // dots 1,3,4,5,6
        /* 'Z'  0x5A */ 0x35, // dots 1,3,5,6
        /* '['  0x5B */ 0x27, // dots 1,2,3,6  (same as V)
        /* '\\' 0x5C */ 0x0C, // dots 3,4
        /* ']'  0x5D */ 0x0E, // dots 2,3,4    (same as S)
        /* '^'  0x5E */ 0x23, // dots 1,2,6
        /* '_'  0x5F */ 0x38, // dots 4,5,6
        /* '`'  0x60 */ 0x01, // dot 1
        /* 'a'  0x61 */ 0x01,
        /* 'b'  0x62 */ 0x03,
        /* 'c'  0x63 */ 0x09,
        /* 'd'  0x64 */ 0x19,
        /* 'e'  0x65 */ 0x11,
        /* 'f'  0x66 */ 0x0B,
        /* 'g'  0x67 */ 0x1B,
        /* 'h'  0x68 */ 0x13,
        /* 'i'  0x69 */ 0x0A,
        /* 'j'  0x6A */ 0x1A,
        /* 'k'  0x6B */ 0x05,
        /* 'l'  0x6C */ 0x07,
        /* 'm'  0x6D */ 0x0D,
        /* 'n'  0x6E */ 0x1D,
        /* 'o'  0x6F */ 0x15,
        /* 'p'  0x70 */ 0x0F,
        /* 'q'  0x71 */ 0x1F,
        /* 'r'  0x72 */ 0x17,
        /* 's'  0x73 */ 0x0E,
        /* 't'  0x74 */ 0x1E,
        /* 'u'  0x75 */ 0x25,
        /* 'v'  0x76 */ 0x27,
        /* 'w'  0x77 */ 0x3A,
        /* 'x'  0x78 */ 0x2D,
        /* 'y'  0x79 */ 0x3D,
        /* 'z'  0x7A */ 0x35,
        /* '{'  0x7B */ 0x2F, // dots 1,2,3,4,6
        /* '|'  0x7C */ 0x56, // dots 2,3,5,7  (8-dot extension)
        /* '}'  0x7D */ 0x3B, // dots 1,2,4,5,6
        /* '~'  0x7E */ 0x21, // dots 1,6
    };
    unsigned idx = static_cast<unsigned char>(c);
    if (idx < 0x20u || idx > 0x7Eu) return 0x00;
    return kFull[idx - 0x20u];
}

// ── Bulk helpers ──────────────────────────────────────────────────────────────

// Convert BrailleCell vector → UTF-8 string of Unicode Braille characters
inline std::string CellsToUtf8(const std::vector<BrailleCell>& cells) {
    std::string out;
    out.reserve(cells.size() * 3); // U+2800-U+28FF = 3 UTF-8 bytes each
    for (BrailleCell c : cells) {
        AppendUtf8(out, CellToUnicode(c));
    }
    return out;
}

} // namespace Braille
} // namespace AccessOS
