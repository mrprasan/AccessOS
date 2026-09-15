// Test_BrailleSubsystem.cpp — ACCESSOS-021 Braille Output Subsystem tests
// Covers: BrailleCells, BrailleTranslator, BrailleDisplayManager

#include <gtest/gtest.h>
#include "../../src/Core/Braille/BrailleCells.h"
#include "../../src/Core/Braille/BrailleTranslator.h"
#include "../../src/Core/Braille/IBrailleDisplay.h"
#include "../../src/Core/Braille/BrailleDisplayManager.h"

using namespace AccessOS::Braille;

// ── Stub display for testing ──────────────────────────────────────────────────

class StubBrailleDisplay : public IBrailleDisplay {
public:
    explicit StubBrailleDisplay(uint32_t cells = 40) {
        m_caps.cellCount      = cells;
        m_caps.hasCursor      = true;
        m_caps.hasRoutingKeys = true;
        m_caps.hasPanKeys     = true;
        m_caps.deviceName     = "StubDisplay";
    }

    bool Connect()    override { m_connected = true;  return true; }
    void Disconnect() override { m_connected = false; }
    bool IsConnected() const override { return m_connected; }

    BrailleDisplayCaps GetCapabilities() const override { return m_caps; }

    bool Write(const std::vector<BrailleCell>& cells) override {
        if (!m_connected) return false;
        m_lastWrite = cells;
        ++m_writeCount;
        return true;
    }

    bool PanLeft()  override { return m_connected; }
    bool PanRight() override { return m_connected; }

    bool SetCursorCell(uint32_t idx) override {
        if (!m_connected) return false;
        m_cursorCell = idx;
        return true;
    }
    uint32_t GetCursorCell() const override { return m_cursorCell; }

    // Test accessors
    const std::vector<BrailleCell>& LastWrite() const { return m_lastWrite; }
    int WriteCount() const { return m_writeCount; }

private:
    BrailleDisplayCaps       m_caps;
    bool                     m_connected  = false;
    std::vector<BrailleCell> m_lastWrite;
    int                      m_writeCount = 0;
    uint32_t                 m_cursorCell = UINT32_MAX;
};

// ── BrailleCells tests ────────────────────────────────────────────────────────

TEST(BrailleCells, CellToUnicodeEmptyCell) {
    EXPECT_EQ(CellToUnicode(0x00), U'\u2800');
}

TEST(BrailleCells, CellToUnicodeFullCell) {
    EXPECT_EQ(CellToUnicode(0xFF), static_cast<char32_t>(0x28FFu));
}

TEST(BrailleCells, UnicodeToCellBitsInRange) {
    auto result = UnicodeToCellBits(0x2801u);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x01u);
}

TEST(BrailleCells, UnicodeToCellBitsOutOfRange) {
    EXPECT_FALSE(UnicodeToCellBits(0x0041u).has_value()); // 'A'
    EXPECT_FALSE(UnicodeToCellBits(0x2900u).has_value()); // just above
}

TEST(BrailleCells, HasDotTrueFalse) {
    BrailleCell cell = 0x01; // dot 1 set
    EXPECT_TRUE(HasDot(cell, 1));
    EXPECT_FALSE(HasDot(cell, 2));
}

TEST(BrailleCells, HasDotOutOfRange) {
    EXPECT_FALSE(HasDot(0xFF, 0));
    EXPECT_FALSE(HasDot(0xFF, 9));
}

TEST(BrailleCells, SetDot) {
    BrailleCell cell = 0x00;
    cell = SetDot(cell, 1);
    EXPECT_EQ(cell, 0x01u);
    cell = SetDot(cell, 3);
    EXPECT_EQ(cell, 0x05u); // dots 1,3
}

TEST(BrailleCells, ClearDot) {
    BrailleCell cell = 0x07; // dots 1,2,3
    cell = ClearDot(cell, 2);
    EXPECT_EQ(cell, 0x05u); // dots 1,3
}

TEST(BrailleCells, SetDotOutOfRange) {
    BrailleCell cell = 0x00;
    EXPECT_EQ(SetDot(cell, 0), 0x00u);
    EXPECT_EQ(SetDot(cell, 9), 0x00u);
}

TEST(BrailleCells, BrfToCellLowercaseA) {
    EXPECT_EQ(BrfToCell('a'), 0x01u); // dot 1
}

TEST(BrailleCells, BrfToCellLowercaseLetters) {
    // Standard Grade 1 alphabet: a=0x01, b=0x03, c=0x09, d=0x19, e=0x11
    EXPECT_EQ(BrfToCell('a'), 0x01u);
    EXPECT_EQ(BrfToCell('b'), 0x03u);
    EXPECT_EQ(BrfToCell('c'), 0x09u);
    EXPECT_EQ(BrfToCell('d'), 0x19u);
    EXPECT_EQ(BrfToCell('e'), 0x11u);
}

TEST(BrailleCells, BrfToCellMoreLetters) {
    EXPECT_EQ(BrfToCell('k'), 0x05u); // dots 1,3
    EXPECT_EQ(BrfToCell('l'), 0x07u); // dots 1,2,3
    EXPECT_EQ(BrfToCell('w'), 0x3Au); // dots 2,4,5,6
}

TEST(BrailleCells, BrfToCellSpace) {
    EXPECT_EQ(BrfToCell(' '), 0x00u);
}

TEST(BrailleCells, BrfToCellOutOfRange) {
    EXPECT_EQ(BrfToCell('\x01'), 0x00u);
    EXPECT_EQ(BrfToCell('\x7F'), 0x00u);
}

TEST(BrailleCells, BrfToCellUppercaseMatchesLower) {
    // Uppercase letters map to same cell bits as lowercase in NABCC
    EXPECT_EQ(BrfToCell('A'), BrfToCell('a'));
    EXPECT_EQ(BrfToCell('Z'), BrfToCell('z'));
}

TEST(BrailleCells, CellsToUtf8EmptyCell) {
    std::vector<BrailleCell> cells = { 0x00 };
    std::string utf8 = CellsToUtf8(cells);
    // U+2800 = 0xE2 0xA0 0x80
    ASSERT_EQ(utf8.size(), 3u);
    EXPECT_EQ((unsigned char)utf8[0], 0xE2u);
    EXPECT_EQ((unsigned char)utf8[1], 0xA0u);
    EXPECT_EQ((unsigned char)utf8[2], 0x80u);
}

TEST(BrailleCells, CellsToUtf8Dot1) {
    std::vector<BrailleCell> cells = { 0x01 }; // U+2801
    std::string utf8 = CellsToUtf8(cells);
    ASSERT_EQ(utf8.size(), 3u);
    EXPECT_EQ((unsigned char)utf8[0], 0xE2u);
    EXPECT_EQ((unsigned char)utf8[1], 0xA0u);
    EXPECT_EQ((unsigned char)utf8[2], 0x81u);
}

TEST(BrailleCells, CellsToUtf8Multiple) {
    std::vector<BrailleCell> cells = { 0x01, 0x03 }; // 'a', 'b'
    EXPECT_EQ(CellsToUtf8(cells).size(), 6u); // 2 × 3-byte sequences
}

// ── BrailleTranslator tests ───────────────────────────────────────────────────

class BrailleTranslatorTest : public ::testing::Test {
protected:
    BrailleTranslator tr;
};

TEST_F(BrailleTranslatorTest, EmptyString) {
    EXPECT_TRUE(tr.Translate("").empty());
}

TEST_F(BrailleTranslatorTest, SingleLowercaseLetter) {
    auto cells = tr.Translate("a");
    ASSERT_EQ(cells.size(), 1u);
    EXPECT_EQ(cells[0], 0x01u); // dot 1
}

TEST_F(BrailleTranslatorTest, Word_hello) {
    // h=0x13, e=0x11, l=0x07, l=0x07, o=0x15
    auto cells = tr.Translate("hello");
    ASSERT_EQ(cells.size(), 5u);
    EXPECT_EQ(cells[0], 0x13u); // h
    EXPECT_EQ(cells[1], 0x11u); // e
    EXPECT_EQ(cells[2], 0x07u); // l
    EXPECT_EQ(cells[3], 0x07u); // l
    EXPECT_EQ(cells[4], 0x15u); // o
}

TEST_F(BrailleTranslatorTest, SpaceBecomesEmptyCell) {
    auto cells = tr.Translate("a b");
    ASSERT_EQ(cells.size(), 3u);
    EXPECT_EQ(cells[1], 0x00u); // space
}

TEST_F(BrailleTranslatorTest, UppercaseEmitsCapitalIndicator) {
    // "A" → capital indicator (0x20) + cell for 'a' (0x01)
    auto cells = tr.Translate("A");
    ASSERT_EQ(cells.size(), 2u);
    EXPECT_EQ(cells[0], kCapitalIndicator);
    EXPECT_EQ(cells[1], 0x01u);
}

TEST_F(BrailleTranslatorTest, UppercaseWordHello) {
    // "Hello" → cap+h e l l o
    auto cells = tr.Translate("Hello");
    ASSERT_EQ(cells.size(), 6u); // cap indicator + 5 letters
    EXPECT_EQ(cells[0], kCapitalIndicator);
    EXPECT_EQ(cells[1], 0x13u); // h
}

TEST_F(BrailleTranslatorTest, DigitRunSingleNumberIndicator) {
    // "123" → number indicator + cells for 1,2,3
    auto cells = tr.Translate("123");
    ASSERT_EQ(cells.size(), 4u); // 1 indicator + 3 digits
    EXPECT_EQ(cells[0], kNumberIndicator);
}

TEST_F(BrailleTranslatorTest, DigitSingleZero) {
    auto cells = tr.Translate("0");
    ASSERT_EQ(cells.size(), 2u); // indicator + digit
    EXPECT_EQ(cells[0], kNumberIndicator);
    EXPECT_EQ(cells[1], BrfToCell('0'));
}

TEST_F(BrailleTranslatorTest, MultipleDigitRunsGetSeparateIndicators) {
    // "1 2" → indicator+1, space, indicator+2
    auto cells = tr.Translate("1 2");
    // indicator(1) + digit(1) + space(1) + indicator(1) + digit(1) = 5
    ASSERT_EQ(cells.size(), 5u);
    EXPECT_EQ(cells[0], kNumberIndicator);
    EXPECT_EQ(cells[2], 0x00u); // space
    EXPECT_EQ(cells[3], kNumberIndicator);
}

TEST_F(BrailleTranslatorTest, MixedTextAndNumbers) {
    // "a1" → 'a' cell, number_indicator, '1' cell
    auto cells = tr.Translate("a1");
    ASSERT_EQ(cells.size(), 3u);
    EXPECT_EQ(cells[0], BrfToCell('a'));
    EXPECT_EQ(cells[1], kNumberIndicator);
    EXPECT_EQ(cells[2], BrfToCell('1'));
}

TEST_F(BrailleTranslatorTest, TranslateToUnicodeReturnsNonEmpty) {
    std::string u = tr.TranslateToUnicode("hi");
    EXPECT_FALSE(u.empty());
    // Each cell is 3 UTF-8 bytes (U+2800-U+28FF)
    auto cells = tr.Translate("hi");
    EXPECT_EQ(u.size(), cells.size() * 3u);
}

TEST_F(BrailleTranslatorTest, ControlCharsSkipped) {
    // "\x01\x02" should produce no cells
    EXPECT_TRUE(tr.Translate("\x01\x02").empty());
}

TEST_F(BrailleTranslatorTest, TabProducesEmptyCell) {
    auto cells = tr.Translate("\t");
    ASSERT_EQ(cells.size(), 1u);
    EXPECT_EQ(cells[0], 0x00u);
}

// ── StubBrailleDisplay tests ──────────────────────────────────────────────────

TEST(StubDisplay, ConnectDisconnect) {
    StubBrailleDisplay disp(40);
    EXPECT_FALSE(disp.IsConnected());
    EXPECT_TRUE(disp.Connect());
    EXPECT_TRUE(disp.IsConnected());
    disp.Disconnect();
    EXPECT_FALSE(disp.IsConnected());
}

TEST(StubDisplay, WriteWhenDisconnectedReturnsFalse) {
    StubBrailleDisplay disp(40);
    EXPECT_FALSE(disp.Write({ 0x01 }));
}

TEST(StubDisplay, WriteRecordsData) {
    StubBrailleDisplay disp(40);
    disp.Connect();
    std::vector<BrailleCell> cells = { 0x01, 0x03, 0x09 };
    EXPECT_TRUE(disp.Write(cells));
    EXPECT_EQ(disp.LastWrite(), cells);
    EXPECT_EQ(disp.WriteCount(), 1);
}

TEST(StubDisplay, CursorCell) {
    StubBrailleDisplay disp(40);
    disp.Connect();
    EXPECT_TRUE(disp.SetCursorCell(5));
    EXPECT_EQ(disp.GetCursorCell(), 5u);
}

// ── BrailleDisplayManager tests ──────────────────────────────────────────────

class BrailleDisplayManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_stub = std::make_shared<StubBrailleDisplay>(8); // 8-cell display for pan tests
        m_stub->Connect();
        m_mgr.SetDisplay(m_stub);
    }

    std::shared_ptr<StubBrailleDisplay> m_stub;
    BrailleDisplayManager               m_mgr;
};

TEST_F(BrailleDisplayManagerTest, HasDisplayTrue) {
    EXPECT_TRUE(m_mgr.HasDisplay());
}

TEST_F(BrailleDisplayManagerTest, HasDisplayFalseWhenNoDisplay) {
    BrailleDisplayManager mgr;
    EXPECT_FALSE(mgr.HasDisplay());
}

TEST_F(BrailleDisplayManagerTest, WriteTextSendsToDisplay) {
    EXPECT_TRUE(m_mgr.WriteText("hello"));
    EXPECT_EQ(m_stub->WriteCount(), 1);
}

TEST_F(BrailleDisplayManagerTest, WriteTextSetsBuffer) {
    m_mgr.WriteText("hello");
    EXPECT_FALSE(m_mgr.GetBuffer().empty());
}

TEST_F(BrailleDisplayManagerTest, WriteTextResetsPanOffset) {
    m_mgr.WriteText("abcdefghijklmnop"); // > 8 cells
    m_mgr.PanRight();
    EXPECT_GT(m_mgr.GetPanOffset(), 0u);
    m_mgr.WriteText("hi");
    EXPECT_EQ(m_mgr.GetPanOffset(), 0u);
}

TEST_F(BrailleDisplayManagerTest, WriteCellsDirectly) {
    std::vector<BrailleCell> cells = { 0x01, 0x03, 0x09 };
    EXPECT_TRUE(m_mgr.WriteCells(cells));
    EXPECT_EQ(m_stub->WriteCount(), 1);
    EXPECT_EQ(m_mgr.GetBuffer(), cells);
}

TEST_F(BrailleDisplayManagerTest, PanRightAdvancesOffset) {
    // Write 16 cells worth of text to a 8-cell display
    m_mgr.WriteText("abcdefghijklmnop"); // 16 lowercase letters = 16 cells
    uint32_t before = m_mgr.GetPanOffset();
    bool panned = m_mgr.PanRight();
    EXPECT_TRUE(panned);
    EXPECT_EQ(m_mgr.GetPanOffset(), before + 8u);
}

TEST_F(BrailleDisplayManagerTest, PanLeftAtStartReturnsFalse) {
    m_mgr.WriteText("hello");
    EXPECT_FALSE(m_mgr.PanLeft()); // already at offset 0
}

TEST_F(BrailleDisplayManagerTest, PanRightAtEndReturnsFalse) {
    m_mgr.WriteText("hi"); // 2 cells < 8 display cells, can't pan right
    EXPECT_FALSE(m_mgr.PanRight());
}

TEST_F(BrailleDisplayManagerTest, PanRightThenLeft) {
    m_mgr.WriteText("abcdefghijklmnop"); // 16 cells
    m_mgr.PanRight();
    EXPECT_EQ(m_mgr.GetPanOffset(), 8u);
    EXPECT_TRUE(m_mgr.PanLeft());
    EXPECT_EQ(m_mgr.GetPanOffset(), 0u);
}

TEST_F(BrailleDisplayManagerTest, ViewportFilledWithBlankBeyondBuffer) {
    // Buffer of 3 cells on 8-cell display → last 5 cells should be 0x00
    m_mgr.WriteCells({ 0x01, 0x03, 0x09 });
    const auto& written = m_stub->LastWrite();
    ASSERT_EQ(written.size(), 8u);
    EXPECT_EQ(written[0], 0x01u);
    EXPECT_EQ(written[1], 0x03u);
    EXPECT_EQ(written[2], 0x09u);
    for (size_t j = 3; j < 8; ++j) {
        EXPECT_EQ(written[j], 0x00u) << "cell " << j << " should be blank";
    }
}

TEST_F(BrailleDisplayManagerTest, SetCursorCellPansViewport) {
    // 16-cell buffer, 8-cell display; cursor at cell 10 should pan to offset 8
    m_mgr.WriteText("abcdefghijklmnop"); // 16 cells
    m_mgr.SetCursorCell(10);
    EXPECT_EQ(m_mgr.GetPanOffset(), 8u);
}

TEST_F(BrailleDisplayManagerTest, FlushWritesCurrentViewport) {
    m_mgr.WriteCells({ 0x01 });
    int before = m_stub->WriteCount();
    m_mgr.Flush();
    EXPECT_EQ(m_stub->WriteCount(), before + 1);
}

TEST_F(BrailleDisplayManagerTest, NoDisplayWriteReturnsFalse) {
    BrailleDisplayManager mgr;
    EXPECT_FALSE(mgr.WriteText("hello"));
    EXPECT_FALSE(mgr.WriteCells({ 0x01 }));
    EXPECT_FALSE(mgr.PanLeft());
    EXPECT_FALSE(mgr.PanRight());
    EXPECT_FALSE(mgr.Flush());
}

TEST_F(BrailleDisplayManagerTest, GetDisplayReturnsAttached) {
    EXPECT_EQ(m_mgr.GetDisplay(), m_stub);
}
