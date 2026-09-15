// HidBrailleDisplay.cpp — Win32 HID Braille display driver (ACCESSOS-036)

#include "HidBrailleDisplay.h"
#include <Windows.h>
#include <setupapi.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <devguid.h>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "hid.lib")

#include <vector>
#include <string>
#include <algorithm>
#include <cstring>

namespace AccessOS {
namespace Braille {

// ── Known device table ────────────────────────────────────────────────────────
// VID/PID pairs for commonly used refreshable Braille displays.
// cellCount is the default; actual count is queried from the device on connect.

const HidDeviceId HidBrailleDisplay::s_knownDevices[] = {
    // HumanWare BrailleNote Touch
    { 0x1C71, 0xC005, 32, "HumanWare BrailleNote Touch 32" },
    { 0x1C71, 0xC006, 18, "HumanWare BrailleNote Touch 18" },
    // HumanWare Brailliant BI
    { 0x1C71, 0xC021, 40, "HumanWare Brailliant BI 40" },
    { 0x1C71, 0xC022, 20, "HumanWare Brailliant BI 20" },
    // Freedom Scientific Focus 40 Blue
    { 0x0F4E, 0x0114, 40, "Freedom Scientific Focus 40 Blue" },
    { 0x0F4E, 0x0112, 14, "Freedom Scientific Focus 14 Blue" },
    // HIMS Braille EDGE
    { 0x045E, 0x930A, 40, "HIMS Braille EDGE 40" },
    // Papenmeier BRAILLEX
    { 0x0403, 0xF208, 80, "Papenmeier BRAILLEX 80" },
};

const size_t HidBrailleDisplay::s_knownDeviceCount =
    sizeof(s_knownDevices) / sizeof(s_knownDevices[0]);

// ── Construction / destruction ────────────────────────────────────────────────

HidBrailleDisplay::HidBrailleDisplay(std::wstring devicePath, uint32_t cellCount)
    : m_devicePath(std::move(devicePath))
    , m_hDevice(INVALID_HANDLE_VALUE)
    , m_cellCount(cellCount)
    , m_cursorCell(UINT32_MAX)
{}

HidBrailleDisplay::~HidBrailleDisplay() {
    Disconnect();
}

// ── IBrailleDisplay: lifecycle ────────────────────────────────────────────────

bool HidBrailleDisplay::Connect() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_hDevice != INVALID_HANDLE_VALUE) return true; // already connected

    if (m_devicePath.empty()) {
        if (!AutoDiscover()) return false;
    }

    m_hDevice = ::CreateFileW(
        m_devicePath.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        nullptr);

    if (m_hDevice == INVALID_HANDLE_VALUE) {
        m_devicePath.clear();
        return false;
    }

    // Query actual cell count from HID descriptor if possible
    HIDD_ATTRIBUTES attrs{};
    attrs.Size = sizeof(attrs);
    if (::HidD_GetAttributes(m_hDevice, &attrs)) {
        // Look up in known table to refine cell count
        for (size_t i = 0; i < s_knownDeviceCount; ++i) {
            if (s_knownDevices[i].vendorId  == attrs.VendorID &&
                s_knownDevices[i].productId == attrs.ProductID) {
                m_cellCount = s_knownDevices[i].cellCount;
                break;
            }
        }
    }

    return true;
}

void HidBrailleDisplay::Disconnect() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_hDevice != INVALID_HANDLE_VALUE) {
        ::CloseHandle(m_hDevice);
        m_hDevice = INVALID_HANDLE_VALUE;
    }
}

bool HidBrailleDisplay::IsConnected() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_hDevice != INVALID_HANDLE_VALUE;
}

// ── IBrailleDisplay: capabilities ────────────────────────────────────────────

BrailleDisplayCaps HidBrailleDisplay::GetCapabilities() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    BrailleDisplayCaps caps;
    caps.cellCount       = m_cellCount;
    caps.hasCursor       = true;
    caps.hasRoutingKeys  = true;
    caps.hasPanKeys      = true;

    if (m_hDevice != INVALID_HANDLE_VALUE) {
        // Attempt to read manufacturer/product strings for deviceName
        wchar_t buf[128] = {};
        if (::HidD_GetProductString(m_hDevice, buf, sizeof(buf))) {
            int len = ::WideCharToMultiByte(CP_UTF8, 0, buf, -1,
                                             nullptr, 0, nullptr, nullptr);
            if (len > 0) {
                caps.deviceName.resize(static_cast<size_t>(len) - 1);
                ::WideCharToMultiByte(CP_UTF8, 0, buf, -1,
                                       caps.deviceName.data(), len, nullptr, nullptr);
            }
        }
    }
    if (caps.deviceName.empty()) caps.deviceName = "HID Braille Display";
    return caps;
}

// ── IBrailleDisplay: output ───────────────────────────────────────────────────

bool HidBrailleDisplay::Write(const std::vector<BrailleCell>& cells) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_hDevice == INVALID_HANDLE_VALUE) return false;
    auto report = BuildCellReport(cells);
    return WriteReport(report);
}

bool HidBrailleDisplay::PanLeft() {
    // Pan commands are device-specific; send a generic pan-left key report.
    // Report ID 0x02 byte 0x01 = pan left (HumanWare convention).
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_hDevice == INVALID_HANDLE_VALUE) return false;
    std::vector<uint8_t> report = { 0x02, 0x01 };
    return WriteReport(report);
}

bool HidBrailleDisplay::PanRight() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_hDevice == INVALID_HANDLE_VALUE) return false;
    std::vector<uint8_t> report = { 0x02, 0x02 };
    return WriteReport(report);
}

bool HidBrailleDisplay::SetCursorCell(uint32_t cellIndex) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cursorCell = cellIndex;
    return true;
}

uint32_t HidBrailleDisplay::GetCursorCell() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_cursorCell;
}

// ── HID-specific ──────────────────────────────────────────────────────────────

// static
std::vector<std::pair<std::wstring, BrailleDisplayCaps>>
HidBrailleDisplay::EnumerateDevices() {
    std::vector<std::pair<std::wstring, BrailleDisplayCaps>> result;

    GUID hidGuid;
    ::HidD_GetHidGuid(&hidGuid);

    HDEVINFO devInfo = ::SetupDiGetClassDevsW(
        &hidGuid, nullptr, nullptr,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (devInfo == INVALID_HANDLE_VALUE) return result;

    SP_DEVICE_INTERFACE_DATA ifData{};
    ifData.cbSize = sizeof(ifData);

    for (DWORD idx = 0;
         ::SetupDiEnumDeviceInterfaces(devInfo, nullptr, &hidGuid, idx, &ifData);
         ++idx) {

        // Get the required buffer size
        DWORD requiredSize = 0;
        ::SetupDiGetDeviceInterfaceDetailW(devInfo, &ifData,
                                           nullptr, 0, &requiredSize, nullptr);
        if (requiredSize == 0) continue;

        std::vector<uint8_t> buf(requiredSize);
        auto* detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(buf.data());
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

        if (!::SetupDiGetDeviceInterfaceDetailW(devInfo, &ifData,
                detail, requiredSize, nullptr, nullptr)) continue;

        std::wstring path = detail->DevicePath;

        // Open the device briefly to read attributes
        HANDLE h = ::CreateFileW(path.c_str(),
                                  GENERIC_READ,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  nullptr,
                                  OPEN_EXISTING,
                                  0, nullptr);
        if (h == INVALID_HANDLE_VALUE) continue;

        HIDD_ATTRIBUTES attrs{};
        attrs.Size = sizeof(attrs);
        bool matched = false;
        BrailleDisplayCaps caps;

        if (::HidD_GetAttributes(h, &attrs)) {
            for (size_t k = 0; k < s_knownDeviceCount; ++k) {
                if (s_knownDevices[k].vendorId  == attrs.VendorID &&
                    s_knownDevices[k].productId == attrs.ProductID) {
                    caps.cellCount = s_knownDevices[k].cellCount;
                    caps.deviceName= s_knownDevices[k].name;
                    caps.hasCursor = caps.hasRoutingKeys = caps.hasPanKeys = true;
                    matched = true;
                    break;
                }
            }
        }
        ::CloseHandle(h);

        if (matched) result.emplace_back(std::move(path), caps);
    }

    ::SetupDiDestroyDeviceInfoList(devInfo);
    return result;
}

// ── protected helpers ─────────────────────────────────────────────────────────

std::vector<uint8_t>
HidBrailleDisplay::BuildCellReport(const std::vector<BrailleCell>& cells) const {
    // Report layout: [ReportID=0x01][cell0][cell1]…[cellN-1]
    // Pad to m_cellCount; truncate if longer.
    std::vector<uint8_t> report;
    report.reserve(1 + m_cellCount);
    report.push_back(0x01); // Report ID

    for (uint32_t i = 0; i < m_cellCount; ++i) {
        if (i < static_cast<uint32_t>(cells.size())) {
            report.push_back(cells[i]);
        } else {
            report.push_back(0x00); // blank cell
        }
    }
    return report;
}

// ── private helpers ───────────────────────────────────────────────────────────

bool HidBrailleDisplay::WriteReport(const std::vector<uint8_t>& report) {
    if (m_hDevice == INVALID_HANDLE_VALUE || report.empty()) return false;

    DWORD written = 0;
    OVERLAPPED ov{};
    ov.hEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!ov.hEvent) return false;

    BOOL ok = ::WriteFile(m_hDevice,
                           report.data(),
                           static_cast<DWORD>(report.size()),
                           &written, &ov);

    if (!ok && ::GetLastError() == ERROR_IO_PENDING) {
        // Wait up to 500 ms for async write to complete
        DWORD wait = ::WaitForSingleObject(ov.hEvent, 500);
        if (wait == WAIT_OBJECT_0) {
            ok = ::GetOverlappedResult(m_hDevice, &ov, &written, FALSE);
        } else {
            ::CancelIo(m_hDevice);
            ok = FALSE;
        }
    }

    ::CloseHandle(ov.hEvent);
    return ok == TRUE;
}

bool HidBrailleDisplay::AutoDiscover() {
    auto devices = EnumerateDevices();
    if (devices.empty()) return false;
    m_devicePath = devices[0].first;
    m_cellCount  = devices[0].second.cellCount;
    return true;
}

} // namespace Braille
} // namespace AccessOS
