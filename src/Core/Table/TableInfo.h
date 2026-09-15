#pragma once
// TableInfo.h — Table / Grid structural model (ACCESSOS-025)
//
// Represents a snapshot of an accessible table/grid element.
// Populated from UIA TablePattern / GridPattern properties.

#include <string>
#include <vector>
#include <cstdint>

namespace AccessOS {
namespace Table {

// A single table cell snapshot
struct TableCell {
    std::string text;           // inner text / accessible name
    std::string role;           // "columnheader", "rowheader", "cell", "gridcell"
    uint32_t    row    = 0;     // 0-based row index
    uint32_t    col    = 0;     // 0-based column index
    uint32_t    rowSpan= 1;
    uint32_t    colSpan= 1;
    bool        isRowHeader    = false;
    bool        isColumnHeader = false;
};

// Full table structure snapshot
struct TableInfo {
    uint32_t                       rowCount    = 0;
    uint32_t                       colCount    = 0;
    std::string                    caption;     // table caption / accessible name
    std::string                    summary;     // aria-describedby / UIA HelpText
    std::vector<TableCell>         cells;       // row-major order
    bool                           hasColumnHeaders = false;
    bool                           hasRowHeaders    = false;

    bool IsEmpty() const noexcept {
        return rowCount == 0 || colCount == 0;
    }

    // Retrieve cell at (row, col); returns nullptr if out of range
    const TableCell* CellAt(uint32_t row, uint32_t col) const {
        for (const auto& c : cells) {
            if (c.row == row && c.col == col) return &c;
        }
        return nullptr;
    }

    // Return all cells that are column headers (row 0 or isColumnHeader flag)
    std::vector<const TableCell*> ColumnHeaders() const {
        std::vector<const TableCell*> out;
        for (const auto& c : cells) {
            if (c.isColumnHeader) out.push_back(&c);
        }
        return out;
    }

    // Return all cells that are row headers (col 0 or isRowHeader flag)
    std::vector<const TableCell*> RowHeaders() const {
        std::vector<const TableCell*> out;
        for (const auto& c : cells) {
            if (c.isRowHeader) out.push_back(&c);
        }
        return out;
    }
};

} // namespace Table
} // namespace AccessOS
