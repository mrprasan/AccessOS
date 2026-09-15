# AccessOS — Developer Setup

**Version:** 0.1.0
**Phase:** ACCESSOS-001

---

## Prerequisites

All prerequisites must be installed before building. No undocumented local configuration is required.

| Prerequisite | Version | Purpose |
|---|---|---|
| Windows 11 (or Windows 10 1809+) | Build 17763+ | Target and build platform |
| Visual Studio 2022 | 17.x | C++ compiler (MSVC), MSBuild |
| Windows SDK | 10.0.22621.0+ | Win32, UIA, COM APIs |
| CMake | 3.25+ | Build system |
| Git | 2.x | Source control |

### Visual Studio 2022 Workloads Required

Install these workloads from the Visual Studio Installer:

- **Desktop development with C++**
  - MSVC v143 compiler toolset
  - Windows 10/11 SDK
  - CMake tools for Windows

---

## Clone and Configure

```powershell
git clone https://github.com/your-org/AccessOS.git
cd AccessOS
```

---

## Configure (CMake)

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64
```

This will:
1. Download GoogleTest via FetchContent (requires internet on first run).
2. Generate Visual Studio solution files in `./build/`.
3. Generate `./build/generated/Version.h` from `src/Core/Version.h.in`.

---

## Build

```powershell
cmake --build build --config Debug
```

Or for release:

```powershell
cmake --build build --config Release
```

---

## Run Tests

```powershell
ctest --test-dir build --build-config Debug --output-on-failure
```

Or run the test executable directly:

```powershell
./build/bin/Debug/AccessOSUnitTests.exe
```

---

## External Dependencies

| Library | Version | License | Purpose | Source |
|---|---|---|---|---|
| GoogleTest | v1.14.0 | BSD-3-Clause | Unit testing | https://github.com/google/googletest |

GoogleTest is fetched automatically by CMake FetchContent. No manual installation required.

---

## Known Build Limitations — Phase 0

- The project compiles the core library and unit tests only.
- No executable application exists yet (ACCESSOS-002+).
- No UI project exists yet (ACCESSOS-008).
- No browser extension build exists yet (ACCESSOS-009).
