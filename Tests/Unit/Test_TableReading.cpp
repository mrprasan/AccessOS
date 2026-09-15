// Test_TableReading.cpp — ACCESSOS-025 Table/Grid Reading tests
// Covers: TableInfo model, TableNavigator cursor, boundary clamping, announcements

#include <gtest/gtest.h>
#include "../../src/Core/Table/TableInfo.h"
#include "../../src/Core/Table/TableNavigator.h"

using namespace AccessOS::Table;

// ── Test helpers ──────────────────────────────────────────────────────────────

// Build a simple 3×3 table with col headers in row 0 and row headers in col 0
static TableInfo Make3x3Table() {
    TableInfo t;
    t.rowCount = 3;
    t.colCount = 3;
    t.caption  = "Sales Data";
    t.hasColumnHeaders = true;
    t.hasRowHeaders    = true;

    // Row 0: column headers
    const char* colHdrs[] = { "Region", "Q1", "Q2" };
    for (uint32_t c = 0; c < 3; ++c) {
        TableCell cell;
        cell.row = 0; cell.col = c;
        cell.text = colHdrs[c];
        cell.isColumnHeader = true;
        t.cells.push_back(cell);
    }
    // Row 1: row header + data
    {
        TableCell rh; rh.row=1; rh.col=0; rh.text="North"; rh.isRowHeader=true; t.cells.push_back(rh);
        TableCell d1; d1.row=1; d1.col=1; d1.text="100";   t.cells.push_back(d1);
        TableCell d2; d2.row=1; d2.col=2; d2.text="200";   t.cells.push_back(d2);
    }
    // Row 2: row header + data
    {
        TableCell rh; rh.row=2; rh.col=0; rh.text="South"; rh.isRowHeader=true; t.cells.push_back(rh);
        TableCell d1; d1.row=2; d1.col=1; d1.text="150";   t.cells.push_back(d1);
        TableCell d2; d2.row=2; d2.col=2; d2.text="250";   t.cells.push_back(d2);
    }
    return t;
}

// ── TableInfo tests ───────────────────────────────────────────────────────────

TEST(TableInfo, IsEmptyTrueOnDefault) {
    TableInfo t;
    EXPECT_TRUE(t.IsEmpty());
}

TEST(TableInfo, IsEmptyFalseWithData) {
    TableInfo t = Make3x3Table();
    EXPECT_FALSE(t.IsEmpty());
}

TEST(TableInfo, CellAtValidIndex) {
    TableInfo t = Make3x3Table();
    const TableCell* c = t.CellAt(1, 1);
    ASSERT_NE(c, nullptr);
    EXPECT_EQ(c->text, "100");
}

TEST(TableInfo, CellAtInvalidIndexReturnsNull) {
    TableInfo t = Make3x3Table();
    EXPECT_EQ(t.CellAt(10, 10), nullptr);
}

TEST(TableInfo, ColumnHeadersCount) {
    TableInfo t = Make3x3Table();
    auto headers = t.ColumnHeaders();
    EXPECT_EQ(headers.size(), 3u);
}

TEST(TableInfo, RowHeadersCount) {
    TableInfo t = Make3x3Table();
    auto headers = t.RowHeaders();
    EXPECT_EQ(headers.size(), 2u); // rows 1 and 2 have row headers
}

// ── TableNavigator construction ───────────────────────────────────────────────

TEST(TableNavigator, DefaultNoTable) {
    TableNavigator nav;
    EXPECT_FALSE(nav.HasTable());
    EXPECT_EQ(nav.CurrentCell(), nullptr);
}

TEST(TableNavigator, SetTableInitialisesAtOrigin) {
    TableNavigator nav;
    nav.SetTable(Make3x3Table());
    EXPECT_TRUE(nav.HasTable());
    EXPECT_EQ(nav.Row(), 0u);
    EXPECT_EQ(nav.Col(), 0u);
}

TEST(TableNavigator, CurrentCellAtOrigin) {
    TableNavigator nav;
    nav.SetTable(Make3x3Table());
    const TableCell* c = nav.CurrentCell();
    ASSERT_NE(c, nullptr);
    EXPECT_EQ(c->text, "Region");
}

// ── MoveTo ────────────────────────────────────────────────────────────────────

TEST(TableNavigator, MoveToValidCell) {
    TableNavigator nav;
    nav.SetTable(Make3x3Table());
    auto r = nav.MoveTo(1, 2);
    EXPECT_EQ(r, MoveResult::Ok);
    EXPECT_EQ(nav.Row(), 1u);
    EXPECT_EQ(nav.Col(), 2u);
}

TEST(TableNavigator, MoveToClampsOobRow) {
    TableNavigator nav;
    nav.SetTable(Make3x3Table());
    nav.MoveTo(99, 0);
    EXPECT_EQ(nav.Row(), 2u); // clamped to last row
}

TEST(TableNavigator, MoveToClampsOobCol) {
    TableNavigator nav;
    nav.SetTable(Make3x3Table());
    nav.MoveTo(0, 99);
    EXPECT_EQ(nav.Col(), 2u); // clamped to last col
}

// ── MoveNextCell / MovePrevCell ───────────────────────────────────────────────

TEST(TableNavigator, MoveNextCellAdvancesCol) {
    TableNavigator nav;
    nav.SetTable(Make3x3Table());
    nav.MoveTo(0, 0);
    EXPECT_EQ(nav.MoveNextCell(), MoveResult::Ok);
    EXPECT_EQ(nav.Col(), 1u);
}

TEST(TableNavigator, MoveNextCellWrapsToNextRow) {
    TableNavigator nav;
    nav.SetTable(Make3x3Table());
    nav.MoveTo(0, 2); // last col of row 0
    EXPECT_EQ(nav.MoveNextCell(), MoveResult::Ok);
    EXPECT_EQ(nav.Row(), 1u);
    EXPECT_EQ(nav.Col(), 0u);
}

TEST(TableNavigator, MoveNextCellAtLastCellReturnsBoundary) {
    TableNavigator nav;
    nav.SetTable(Make3x3Table());
    nav.MoveTo(2, 2); // last cell
    EXPECT_EQ(nav.MoveNextCell(), MoveResult::Boundary);
    EXPECT_EQ(nav.Row(), 2u); // position unchanged
    EXPECT_EQ(nav.Col(), 2u);
}

TEST(TableNavigator, MovePrevCellDecreasesCol) {
    TableNavigator nav;
    nav.SetTable(Make3x3Table());
    nav.MoveTo(1, 2);
    EXPECT_EQ(nav.MovePrevCell(), MoveResult::Ok);
    EXPECT_EQ(nav.Col(), 1u);
}

TEST(TableNavigator, MovePrevCellWrapsToEndOfPrevRow) {
    TableNavigator nav;
    nav.SetTable(Make3x3Table());
    nav.MoveTo(1, 0); // first col of row 1
    EXPECT_EQ(nav.MovePrevCell(), MoveResult::Ok);
    EXPECT_EQ(nav.Row(), 0u);
    EXPECT_EQ(nav.Col(), 2u); // last col of row 0
}

TEST(TableNavigator, MovePrevCellAtFirstCellReturnsBoundary) {
    TableNavigator nav;
    nav.SetTable(Make3x3Table());
    nav.MoveTo(0, 0);
    EXPECT_EQ(nav.MovePrevCell(), MoveResult::Boundary);
}

// ── Row / Column navigation ───────────────────────────────────────────────────

TEST(TableNavigator, MoveNextRow) {
    TableNavigator nav;
    nav.SetTable(Make3x3Table());
    nav.MoveTo(0, 1);
    EXPECT_EQ(nav.MoveNextRow(), MoveResult::Ok);
    EXPECT_EQ(nav.Row(), 1u);
    EXPECT_EQ(nav.Col(), 1u); // col preserved
}

TEST(TableNavigator, MoveNextRowAtLastRowReturnsBoundary) {
    TableNavigator nav;
    nav.SetTable(Make3x3Table());
    nav.MoveTo(2, 0);
    EXPECT_EQ(nav.MoveNextRow(), MoveResult::Boundary);
}

TEST(TableNavigator, MovePrevRow) {
    TableNavigator nav;
    nav.SetTable(Make3x3Table());
    nav.MoveTo(2, 1);
    EXPECT_EQ(nav.MovePrevRow(), MoveResult::Ok);
    EXPECT_EQ(nav.Row(), 1u);
}

TEST(TableNavigator, MovePrevRowAtFirstRowReturnsBoundary) {
    TableNavigator nav;
    nav.SetTable(Make3x3Table());
    nav.MoveTo(0, 0);
    EXPECT_EQ(nav.MovePrevRow(), MoveResult::Boundary);
}

TEST(TableNavigator, MoveNextColAndPrevCol) {
    TableNavigator nav;
    nav.SetTable(Make3x3Table());
    nav.MoveTo(1, 0);
    EXPECT_EQ(nav.MoveNextCol(), MoveResult::Ok);
    EXPECT_EQ(nav.Col(), 1u);
    EXPECT_EQ(nav.MovePrevCol(), MoveResult::Ok);
    EXPECT_EQ(nav.Col(), 0u);
}

TEST(TableNavigator, MoveNextColBoundary) {
    TableNavigator nav;
    nav.SetTable(Make3x3Table());
    nav.MoveTo(0, 2);
    EXPECT_EQ(nav.MoveNextCol(), MoveResult::Boundary);
}

TEST(TableNavigator, MovePrevColBoundary) {
    TableNavigator nav;
    nav.SetTable(Make3x3Table());
    nav.MoveTo(0, 0);
    EXPECT_EQ(nav.MovePrevCol(), MoveResult::Boundary);
}

TEST(TableNavigator, MoveRowStartAndEnd) {
    TableNavigator nav;
    nav.SetTable(Make3x3Table());
    nav.MoveTo(1, 2);
    EXPECT_EQ(nav.MoveRowStart(), MoveResult::Ok);
    EXPECT_EQ(nav.Col(), 0u);
    EXPECT_EQ(nav.MoveRowEnd(), MoveResult::Ok);
    EXPECT_EQ(nav.Col(), 2u);
}

TEST(TableNavigator, MoveTableStartAndEnd) {
    TableNavigator nav;
    nav.SetTable(Make3x3Table());
    nav.MoveTo(2, 2);
    EXPECT_EQ(nav.MoveTableStart(), MoveResult::Ok);
    EXPECT_EQ(nav.Row(), 0u); EXPECT_EQ(nav.Col(), 0u);
    EXPECT_EQ(nav.MoveTableEnd(), MoveResult::Ok);
    EXPECT_EQ(nav.Row(), 2u); EXPECT_EQ(nav.Col(), 2u);
}

TEST(TableNavigator, EmptyTableReturnsEmpty) {
    TableNavigator nav;
    TableInfo empty;
    nav.SetTable(empty);
    EXPECT_EQ(nav.MoveNextCell(), MoveResult::Empty);
    EXPECT_EQ(nav.MoveNextRow(),  MoveResult::Empty);
    EXPECT_EQ(nav.MoveTo(0, 0),   MoveResult::Empty);
}

// ── Announcements ─────────────────────────────────────────────────────────────

TEST(TableNavigator, AnnounceTableIncludesCaption) {
    TableNavigator nav;
    nav.SetTable(Make3x3Table());
    std::string s = nav.AnnounceTable();
    EXPECT_NE(s.find("Sales Data"), std::string::npos);
}

TEST(TableNavigator, AnnounceTableIncludesDimensions) {
    TableNavigator nav;
    nav.SetTable(Make3x3Table());
    std::string s = nav.AnnounceTable();
    EXPECT_NE(s.find("3 rows"),    std::string::npos);
    EXPECT_NE(s.find("3 columns"), std::string::npos);
}

TEST(TableNavigator, AnnounceNoHeadersModeNone) {
    TableNavigator nav;
    nav.SetTable(Make3x3Table());
    nav.MoveTo(1, 1);
    std::string s = nav.AnnounceCurrentCell(HeaderMode::None);
    // Should contain cell text "100" and position
    EXPECT_NE(s.find("100"),      std::string::npos);
    EXPECT_NE(s.find("row 2"),    std::string::npos);
    EXPECT_NE(s.find("column 2"), std::string::npos);
}

TEST(TableNavigator, AnnounceWithColumnHeader) {
    TableNavigator nav;
    nav.SetTable(Make3x3Table());
    nav.MoveTo(1, 1); // cell "100", column header "Q1"
    std::string s = nav.AnnounceCurrentCell(HeaderMode::ColumnOnly);
    EXPECT_NE(s.find("Q1"),  std::string::npos);
    EXPECT_NE(s.find("100"), std::string::npos);
}

TEST(TableNavigator, AnnounceWithRowHeader) {
    TableNavigator nav;
    nav.SetTable(Make3x3Table());
    nav.MoveTo(1, 1); // row header "North"
    std::string s = nav.AnnounceCurrentCell(HeaderMode::RowOnly);
    EXPECT_NE(s.find("North"), std::string::npos);
    EXPECT_NE(s.find("100"),   std::string::npos);
}

TEST(TableNavigator, AnnounceWithBothHeaders) {
    TableNavigator nav;
    nav.SetTable(Make3x3Table());
    nav.MoveTo(2, 2); // row header "South", col header "Q2", cell "250"
    std::string s = nav.AnnounceCurrentCell(HeaderMode::Both);
    EXPECT_NE(s.find("Q2"),    std::string::npos);
    EXPECT_NE(s.find("South"), std::string::npos);
    EXPECT_NE(s.find("250"),   std::string::npos);
}

TEST(TableNavigator, AnnounceNoTableReturnsEmpty) {
    TableNavigator nav;
    EXPECT_EQ(nav.AnnounceCurrentCell(), "");
    EXPECT_EQ(nav.AnnounceTable(), "No table");
}
