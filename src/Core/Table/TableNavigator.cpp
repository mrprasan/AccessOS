// TableNavigator.cpp — stateful cursor for table/grid reading (ACCESSOS-025)
#include "TableNavigator.h"
#include <sstream>
#include <algorithm>

namespace AccessOS {
namespace Table {

TableNavigator::TableNavigator() = default;

void TableNavigator::SetTable(const TableInfo& table) {
    m_table    = table;
    m_hasTable = true;
    m_row      = 0;
    m_col      = 0;
}

MoveResult TableNavigator::MoveTo(uint32_t row, uint32_t col) {
    if (!m_hasTable || m_table.IsEmpty()) return MoveResult::Empty;
    m_row = std::min(row, m_table.rowCount - 1);
    m_col = std::min(col, m_table.colCount - 1);
    return MoveResult::Ok;
}

MoveResult TableNavigator::MoveNextCell() {
    if (!m_hasTable || m_table.IsEmpty()) return MoveResult::Empty;
    uint32_t next_col = m_col + 1;
    uint32_t next_row = m_row;
    if (next_col >= m_table.colCount) {
        next_col = 0;
        next_row = m_row + 1;
    }
    if (next_row >= m_table.rowCount) return MoveResult::Boundary;
    m_row = next_row;
    m_col = next_col;
    return MoveResult::Ok;
}

MoveResult TableNavigator::MovePrevCell() {
    if (!m_hasTable || m_table.IsEmpty()) return MoveResult::Empty;
    if (m_col > 0) {
        --m_col;
        return MoveResult::Ok;
    }
    if (m_row == 0) return MoveResult::Boundary;
    --m_row;
    m_col = m_table.colCount - 1;
    return MoveResult::Ok;
}

MoveResult TableNavigator::MoveNextRow() {
    if (!m_hasTable || m_table.IsEmpty()) return MoveResult::Empty;
    if (m_row + 1 >= m_table.rowCount) return MoveResult::Boundary;
    ++m_row;
    return MoveResult::Ok;
}

MoveResult TableNavigator::MovePrevRow() {
    if (!m_hasTable || m_table.IsEmpty()) return MoveResult::Empty;
    if (m_row == 0) return MoveResult::Boundary;
    --m_row;
    return MoveResult::Ok;
}

MoveResult TableNavigator::MoveNextCol() {
    if (!m_hasTable || m_table.IsEmpty()) return MoveResult::Empty;
    if (m_col + 1 >= m_table.colCount) return MoveResult::Boundary;
    ++m_col;
    return MoveResult::Ok;
}

MoveResult TableNavigator::MovePrevCol() {
    if (!m_hasTable || m_table.IsEmpty()) return MoveResult::Empty;
    if (m_col == 0) return MoveResult::Boundary;
    --m_col;
    return MoveResult::Ok;
}

MoveResult TableNavigator::MoveRowStart() {
    if (!m_hasTable || m_table.IsEmpty()) return MoveResult::Empty;
    m_col = 0;
    return MoveResult::Ok;
}

MoveResult TableNavigator::MoveRowEnd() {
    if (!m_hasTable || m_table.IsEmpty()) return MoveResult::Empty;
    m_col = m_table.colCount - 1;
    return MoveResult::Ok;
}

MoveResult TableNavigator::MoveTableStart() {
    if (!m_hasTable || m_table.IsEmpty()) return MoveResult::Empty;
    m_row = 0;
    m_col = 0;
    return MoveResult::Ok;
}

MoveResult TableNavigator::MoveTableEnd() {
    if (!m_hasTable || m_table.IsEmpty()) return MoveResult::Empty;
    m_row = m_table.rowCount - 1;
    m_col = m_table.colCount - 1;
    return MoveResult::Ok;
}

const TableCell* TableNavigator::CurrentCell() const {
    if (!m_hasTable || m_table.IsEmpty()) return nullptr;
    return m_table.CellAt(m_row, m_col);
}

std::string TableNavigator::AnnounceCurrentCell(HeaderMode mode) const {
    if (!m_hasTable || m_table.IsEmpty()) return "";

    const TableCell* cell = m_table.CellAt(m_row, m_col);
    std::string cellText  = cell ? cell->text : "";

    std::string prefix;
    bool needCol = (mode == HeaderMode::ColumnOnly || mode == HeaderMode::Both);
    bool needRow = (mode == HeaderMode::RowOnly    || mode == HeaderMode::Both);

    if (needCol) {
        std::string colHdr = ColumnHeaderText(m_col);
        if (!colHdr.empty()) {
            if (!prefix.empty()) prefix += ", ";
            prefix += colHdr;
        }
    }
    if (needRow) {
        std::string rowHdr = RowHeaderText(m_row);
        if (!rowHdr.empty()) {
            if (!prefix.empty()) prefix += ", ";
            prefix += rowHdr;
        }
    }

    // Position label: "row N, column M"
    std::ostringstream pos;
    pos << "row " << (m_row + 1) << ", column " << (m_col + 1);

    std::string announcement;
    if (!prefix.empty()) announcement = prefix + ". ";
    announcement += cellText;
    announcement += ". ";
    announcement += pos.str();
    return announcement;
}

std::string TableNavigator::AnnounceTable() const {
    if (!m_hasTable) return "No table";
    if (m_table.IsEmpty()) return "Empty table";

    std::ostringstream ss;
    if (!m_table.caption.empty()) ss << m_table.caption << ". ";
    ss << "Table with " << m_table.rowCount << " rows and "
       << m_table.colCount << " columns.";
    return ss.str();
}

// ── private ───────────────────────────────────────────────────────────────────

std::string TableNavigator::ColumnHeaderText(uint32_t col) const {
    // Look for a cell in the same column that is a column header
    for (const auto& c : m_table.cells) {
        if (c.col == col && c.isColumnHeader) return c.text;
    }
    return "";
}

std::string TableNavigator::RowHeaderText(uint32_t row) const {
    for (const auto& c : m_table.cells) {
        if (c.row == row && c.isRowHeader) return c.text;
    }
    return "";
}

} // namespace Table
} // namespace AccessOS
