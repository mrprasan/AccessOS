// HidBrailleDisplay.h — Win32 HID-based Braille display driver (ACCESSOS-036)
//
// Communicates with any USB refreshable Braille display that exposes a
// standard HID interface. Device discovery uses SetupAPI; read/write use
// the Win32 HID API (hid.lib / setupapi.lib).
//
// Braille cells are sent as a single HID output report:
//   Byte 0    — Report ID (0x01 for cell data, per BrailleNote/HumanWare convention)
//   Bytes 1…N — one byte per cell, bit pattern matching BrailleCell::dots
//
// If the display uses a different report layout, subclass and override
// BuildCellReport() / ParseInputReport().
//
// Threading: Connect/Disconnect must be called from one thread.
//            Write/PanLeft/PanRight are internally serialised via m_mutex.

#pragma once
#include "IBrailleDisplay.h"
#include <Windows.h>
#include <mutex>
#include <string>
#include <vector>
#include <cstdint>

namespace AccessOS {
namespace Braille {

// USB Vendor/Product ID pair used to identify a supported Braille display.
struct HidDeviceId {
    uint16_t vendorId;
    uint16_t productId;
    uint32_t cellCount;     // expected cell count for this device
    const char* name;       // human-readable name
};

class HidBrailleDisplay : public IBrailleDisplay {
public:
    // Construct with a specific device path (from EnumerateDevices or supplied).
    // If devicePath is empty, Connect() will auto-discover the first known device.
    explicit HidBrailleDisplay(std::wstring devicePath = L"",
                                uint32_t    cellCount   = 40);
    ~HidBrailleDisplay() override;

    // --- IBrailleDisplay ---
    bool Connect()    override;
    void Disconnect() override;
    bool IsConnected() const override;

    BrailleDisplayCaps GetCapabilities() const override;

    bool Write(const std::vector<BrailleCell>& cells) override;
    bool PanLeft()  override;
    bool PanRight() override;
    bool SetCursorCell(uint32_t cellIndex) override;
    uint32_t GetCursorCell() const override;

    // --- HID-specific ---

    // Enumerate all HID devices that match any entry in the known-device table.
    // Returns a list of (devicePath, caps) pairs.
    static std::vector<std::pair<std::wstring, BrailleDisplayCaps>> EnumerateDevices();

    // Return the device path this instance is using (empty if none).
    const std::wstring& DevicePath() const { return m_devicePath; }

protected:
    // Build a raw HID output report from cells. Default: report-id 0x01 + dot bytes.
    // Override for devices with different report layouts.
    virtual std::vector<uint8_t> BuildCellReport(const std::vector<BrailleCell>& cells) const;

private:
    bool WriteReport(const std::vector<uint8_t>& report);
    bool AutoDiscover();   // scan for a known device and set m_devicePath

    std::wstring            m_devicePath;
    HANDLE                  m_hDevice;
    uint32_t                m_cellCount;
    uint32_t                m_cursorCell;   // UINT32_MAX = not set
    mutable std::mutex      m_mutex;

    // Known device table — add more VID/PID pairs as support is validated.
    static const HidDeviceId s_knownDevices[];
    static const size_t      s_knownDeviceCount;
};

} // namespace Braille
} // namespace AccessOS
