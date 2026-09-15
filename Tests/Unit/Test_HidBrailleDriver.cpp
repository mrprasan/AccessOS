// Test_HidBrailleDriver.cpp — ACCESSOS-036 unit tests
//
// Tests for HidBrailleDisplay (HID Braille Display Driver).
// No real hardware required: tests exercise the class without a connected device,
// verifying state machine and report-building logic.

#include <gtest/gtest.h>
#include "../../src/Core/Braille/HidBrailleDisplay.h"
#include "../../src/Core/Braille/BrailleDisplayManager.h"
#include "../../src/Core/Braille/BrailleCells.h"

using namespace AccessOS::Braille;

// ── Construction / disconnected state ────────────────────────────────────────

TEST(HidBrailleDisplay, DefaultConstructorNotConnected) {
    HidBrailleDisplay disp;
    EXPECT_FALSE(disp.IsConnected());
}

TEST(HidBrailleDisplay, ConstructWithEmptyPathNotConnected) {
    HidBrailleDisplay disp(L"", 40);
    EXPECT_FALSE(disp.IsConnected());
}

TEST(HidBrailleDisplay, ConnectWithInvalidPathReturnsFalse) {
    HidBrailleDisplay disp(L"\\\\?\\hid#invalid#path", 40);
    EXPECT_FALSE(disp.Connect());
    EXPECT_FALSE(disp.IsConnected());
}

TEST(HidBrailleDisplay, DisconnectWhenAlreadyDisconnectedIsNoOp) {
    HidBrailleDisplay disp;
    EXPECT_NO_FATAL_FAILURE(disp.Disconnect());
    EXPECT_FALSE(disp.IsConnected());
}

TEST(HidBrailleDisplay, WriteWhenDisconnectedReturnsFalse) {
    HidBrailleDisplay disp;
    std::vector<BrailleCell> cells(40);
    EXPECT_FALSE(disp.Write(cells));
}

TEST(HidBrailleDisplay, PanLeftWhenDisconnectedReturnsFalse) {
    HidBrailleDisplay disp;
    EXPECT_FALSE(disp.PanLeft());
}

TEST(HidBrailleDisplay, PanRightWhenDisconnectedReturnsFalse) {
    HidBrailleDisplay disp;
    EXPECT_FALSE(disp.PanRight());
}

// ── Cursor cell management (doesn't require hardware) ─────────────────────────

TEST(HidBrailleDisplay, DefaultCursorIsNotSet) {
    HidBrailleDisplay disp;
    EXPECT_EQ(disp.GetCursorCell(), UINT32_MAX);
}

TEST(HidBrailleDisplay, SetCursorCellStoresValue) {
    HidBrailleDisplay disp;
    EXPECT_TRUE(disp.SetCursorCell(5));
    EXPECT_EQ(disp.GetCursorCell(), 5u);
}

TEST(HidBrailleDisplay, SetCursorCellZeroIsValid) {
    HidBrailleDisplay disp;
    EXPECT_TRUE(disp.SetCursorCell(0));
    EXPECT_EQ(disp.GetCursorCell(), 0u);
}

TEST(HidBrailleDisplay, SetCursorCellHighIndexStored) {
    HidBrailleDisplay disp;
    EXPECT_TRUE(disp.SetCursorCell(79));
    EXPECT_EQ(disp.GetCursorCell(), 79u);
}

// ── Capabilities ──────────────────────────────────────────────────────────────

TEST(HidBrailleDisplay, GetCapabilitiesReturnsConfiguredCellCount) {
    HidBrailleDisplay disp(L"", 40);
    auto caps = disp.GetCapabilities();
    EXPECT_EQ(caps.cellCount, 40u);
}

TEST(HidBrailleDisplay, GetCapabilitiesHasCursorTrue) {
    HidBrailleDisplay disp;
    auto caps = disp.GetCapabilities();
    EXPECT_TRUE(caps.hasCursor);
}

TEST(HidBrailleDisplay, GetCapabilitiesHasPanKeysTrue) {
    HidBrailleDisplay disp;
    auto caps = disp.GetCapabilities();
    EXPECT_TRUE(caps.hasPanKeys);
}

TEST(HidBrailleDisplay, GetCapabilitiesHasRoutingKeysTrue) {
    HidBrailleDisplay disp;
    auto caps = disp.GetCapabilities();
    EXPECT_TRUE(caps.hasRoutingKeys);
}

TEST(HidBrailleDisplay, GetCapabilitiesCellCount18) {
    HidBrailleDisplay disp(L"", 18);
    EXPECT_EQ(disp.GetCapabilities().cellCount, 18u);
}

TEST(HidBrailleDisplay, GetCapabilitiesCellCount80) {
    HidBrailleDisplay disp(L"", 80);
    EXPECT_EQ(disp.GetCapabilities().cellCount, 80u);
}

// ── Device path ───────────────────────────────────────────────────────────────

TEST(HidBrailleDisplay, DevicePathPreserved) {
    std::wstring path = L"\\\\?\\hid#test_device#0000";
    HidBrailleDisplay disp(path, 40);
    EXPECT_EQ(disp.DevicePath(), path);
}

TEST(HidBrailleDisplay, DefaultDevicePathEmpty) {
    HidBrailleDisplay disp;
    EXPECT_TRUE(disp.DevicePath().empty());
}

// ── EnumerateDevices (no hardware — just verify it doesn't crash) ─────────────

TEST(HidBrailleDisplay, EnumerateDevicesDoesNotCrash) {
    auto devices = HidBrailleDisplay::EnumerateDevices();
    // May return empty on a machine without braille hardware — that's valid.
    SUCCEED();
}

TEST(HidBrailleDisplay, EnumerateDevicesReturnsVector) {
    auto devices = HidBrailleDisplay::EnumerateDevices();
    // Each found device has a non-empty path and non-zero cell count.
    for (const auto& [path, caps] : devices) {
        EXPECT_FALSE(path.empty());
        EXPECT_GT(caps.cellCount, 0u);
        EXPECT_FALSE(caps.deviceName.empty());
    }
}

// ── Report-building (via write to disconnected — only shape tested) ───────────
// We can probe the report builder indirectly: write returns false when
// disconnected, but the report construction path doesn't crash.

TEST(HidBrailleDisplay, WriteEmptyCellsDisconnectedReturnsFalse) {
    HidBrailleDisplay disp(L"", 4);
    std::vector<BrailleCell> cells;
    EXPECT_FALSE(disp.Write(cells)); // disconnected — no crash
}

TEST(HidBrailleDisplay, WriteSingleCellDisconnectedReturnsFalse) {
    HidBrailleDisplay disp(L"", 1);
    BrailleCell c = 0x41; // dots 1 and 7
    EXPECT_FALSE(disp.Write({ c }));
}

// ── Integration: BrailleDisplayManager with HidBrailleDisplay ────────────────

TEST(HidBrailleDisplayManager, AttachHidDisplayNoConnection) {
    auto hid = std::make_shared<HidBrailleDisplay>(L"", 40);
    BrailleDisplayManager mgr(hid);
    // HasDisplay() requires IsConnected(); without hardware it is false.
    // Verify the display is stored via GetDisplay(), and writes fail gracefully.
    EXPECT_NE(mgr.GetDisplay(), nullptr);
    EXPECT_FALSE(mgr.WriteText("hello"));
}

TEST(HidBrailleDisplayManager, DetachRestoresNoDisplay) {
    auto hid = std::make_shared<HidBrailleDisplay>(L"", 40);
    BrailleDisplayManager mgr(hid);
    mgr.SetDisplay(nullptr);
    EXPECT_EQ(mgr.GetDisplay(), nullptr);
}

TEST(HidBrailleDisplayManager, ReplaceDisplay) {
    BrailleDisplayManager mgr;
    EXPECT_EQ(mgr.GetDisplay(), nullptr);
    auto hid = std::make_shared<HidBrailleDisplay>(L"", 40);
    mgr.SetDisplay(hid);
    EXPECT_NE(mgr.GetDisplay(), nullptr);
}

TEST(HidBrailleDisplayManager, FlushWithNoConnectionReturnsFalse) {
    auto hid = std::make_shared<HidBrailleDisplay>(L"", 40);
    BrailleDisplayManager mgr(hid);
    mgr.WriteText("test");
    EXPECT_FALSE(mgr.Flush()); // no hardware — flush fails gracefully
}

// ── Double connect / double disconnect safety ─────────────────────────────────

TEST(HidBrailleDisplay, DoubleDisconnectIsNoOp) {
    HidBrailleDisplay disp;
    EXPECT_NO_FATAL_FAILURE(disp.Disconnect());
    EXPECT_NO_FATAL_FAILURE(disp.Disconnect());
}

TEST(HidBrailleDisplay, ConnectTwiceWithBadPathStillFalse) {
    HidBrailleDisplay disp(L"\\\\?\\hid#bad", 40);
    EXPECT_FALSE(disp.Connect());
    EXPECT_FALSE(disp.Connect());
}
