#pragma once
// TableNavigator.h — stateful cursor for table/grid reading (ACCESSOS-025)
//
// Maintains a (row, col) cursor within a TableInfo.
// All navigation methods clamp to valid bounds and return false at boundaries.

#include "TableInfo.h"
#include <string>

namespace AccessOS {
namespace Table {

// What headers to include when announcing a cell
enum class HeaderMode : uint8_t {
    None        = 0, // announce cell text only
    ColumnOnly  = 1, // prepend column header
    RowOnly     = 2, // prepend row header
    Both        = 3, // prepend both (column then row)
};

// Result of moving the cursor
enum class MoveResult : uint8_t {
    Ok       = 0, // moved successfully
    Boundary = 1, // already at edge — did not move
    Empty    = 2, // table is empty or not set
};

class TableNavigator {
public:
    TableNavigator();

    // Load / replace the table being navigated (resets cursor to 0,0)
    void SetTable(const TableInfo& table);
    bool HasTable() const noexcept { return m_hasTable; }

    // Current position
    uint32_t Row() const noexcept { return m_row; }
    uint32_t Col() const noexcept { return m_col; }

    // Jump to explicit position (clamped to valid range)
    // Returns Ok or Empty
    MoveResult MoveTo(uint32_t row, uint32_t col);

    // Cell-by-cell navigation (reading order)
    MoveResult MoveNextCell();
    MoveResult MovePrevCell();

    // Row navigation
    MoveResult MoveNextRow();
    MoveResult MovePrevRow();

    // Column navigation
    MoveResult MoveNextCol();
    MoveResult MovePrevCol();

    // Jump to start / end of current row
    MoveResult MoveRowStart();
    MoveResult MoveRowEnd();

    // Jump to first / last row
    MoveResult MoveTableStart();
    MoveResult MoveTableEnd();

    // Get the cell at the current cursor position (nullptr if empty)
    const TableCell* CurrentCell() const;

    // Build announcement text for the current cell given a HeaderMode
    std::string AnnounceCurrentCell(HeaderMode mode = HeaderMode::Both) const;

    // Announce the full table (caption + dimensions)
    std::string AnnounceTable() const;

private:
    TableInfo m_table;
    bool      m_hasTable = false;
    uint32_t  m_row      = 0;
    uint32_t  m_col      = 0;

    // Find column header text for the current column
    std::string ColumnHeaderText(uint32_t col) const;
    // Find row header text for the current row
    std::string RowHeaderText(uint32_t row) const;
};

} // namespace Table
} // namespace AccessOS
