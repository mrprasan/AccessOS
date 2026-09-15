#pragma once
// BrailleTranslator.h — Grade 1 Braille translation (ACCESSOS-021)
//
// Translates UTF-8 text to a sequence of BrailleCells using Grade 1 (uncontracted)
// English Braille with number-indicator support.
//
// Rules implemented:
//   R1  Lowercase a-z → standard alphabet cells
//   R2  Uppercase A-Z → capital indicator (dots 4,6 = 0x28) + letter cell
//   R3  Digits 0-9    → number indicator (dots 3,4,5,6 = 0x3C) + digit cells
//                        (runs of consecutive digits share one indicator)
//   R4  Space         → empty cell (0x00)
//   R5  Printable punctuation → BRF table lookup (BrfToCell)
//   R6  Non-printable / out-of-range → skipped (no cell emitted)

#pragma once
#include "BrailleCells.h"
#include <string>
#include <vector>

namespace AccessOS {
namespace Braille {

// Indicators (6-dot values)
static constexpr BrailleCell kCapitalIndicator = 0x20; // dot 6
static constexpr BrailleCell kNumberIndicator  = 0x3C; // dots 3,4,5,6

class BrailleTranslator {
public:
    // Translate UTF-8 text to Grade 1 Braille cell sequence.
    // Multi-byte UTF-8 characters outside ASCII are skipped (Grade 1 scope).
    std::vector<BrailleCell> Translate(const std::string& text) const;

    // Convenience: translate and return UTF-8 Unicode Braille string (U+2800-U+28FF)
    std::string TranslateToUnicode(const std::string& text) const;

private:
    // Emit capital indicator + cell for uppercase letter
    void EmitUppercase(std::vector<BrailleCell>& out, char c) const;
    // Emit number indicator then consecutive digit cells; advances i past all digits
    void EmitDigitRun(const std::string& text, size_t& i,
                      std::vector<BrailleCell>& out) const;
};

} // namespace Braille
} // namespace AccessOS
