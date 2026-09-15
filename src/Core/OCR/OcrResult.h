#pragma once
// OcrResult.h — OCR structured result model (ACCESSOS-022)
//
// OcrWord  — one recognised word with text, confidence, and bounding rect
// OcrLine  — ordered sequence of words forming one text line
// OcrResult — full page result: lines + merged full text + source rect

#include <string>
#include <vector>
#include <cstdint>

namespace AccessOS {
namespace OCR {

// Axis-aligned integer bounding rectangle in screen / bitmap coordinates
struct OcrRect {
    int32_t x      = 0;
    int32_t y      = 0;
    int32_t width  = 0;
    int32_t height = 0;

    bool IsEmpty() const noexcept { return width <= 0 || height <= 0; }

    bool Contains(int32_t px, int32_t py) const noexcept {
        return px >= x && px < x + width &&
               py >= y && py < y + height;
    }
};

// A single recognised word
struct OcrWord {
    std::string text;
    float       confidence = 0.0f;  // 0.0 – 1.0
    OcrRect     bounds;
};

// A line of text (ordered sequence of words)
struct OcrLine {
    std::vector<OcrWord> words;

    // Concatenate all word texts with single spaces
    std::string Text() const {
        std::string out;
        for (const auto& w : words) {
            if (!out.empty()) out += ' ';
            out += w.text;
        }
        return out;
    }

    // Union bounding rect of all words in this line
    OcrRect Bounds() const {
        if (words.empty()) return {};
        int32_t minX = words[0].bounds.x;
        int32_t minY = words[0].bounds.y;
        int32_t maxX = minX + words[0].bounds.width;
        int32_t maxY = minY + words[0].bounds.height;
        for (size_t i = 1; i < words.size(); ++i) {
            const OcrRect& r = words[i].bounds;
            if (r.x < minX)               minX = r.x;
            if (r.y < minY)               minY = r.y;
            if (r.x + r.width  > maxX)    maxX = r.x + r.width;
            if (r.y + r.height > maxY)    maxY = r.y + r.height;
        }
        return { minX, minY, maxX - minX, maxY - minY };
    }
};

// Full recognition result for one image/region
struct OcrResult {
    std::vector<OcrLine> lines;
    OcrRect              sourceRect;   // region of original bitmap that was processed
    std::string          languageTag;  // e.g. "en-US"
    bool                 succeeded = false;

    // Full text: all lines joined by newlines
    std::string FullText() const {
        std::string out;
        for (const auto& l : lines) {
            if (!out.empty()) out += '\n';
            out += l.Text();
        }
        return out;
    }

    // Total word count across all lines
    size_t WordCount() const {
        size_t n = 0;
        for (const auto& l : lines) n += l.words.size();
        return n;
    }

    bool IsEmpty() const noexcept { return lines.empty(); }
};

} // namespace OCR
} // namespace AccessOS
