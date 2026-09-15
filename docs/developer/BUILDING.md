# Building AccessOS

Developer build guide for AccessOS v1.0.

---

## Prerequisites

| Tool | Version | Notes |
|---|---|---|
| Visual Studio Build Tools 2022 | 17.x | With MSVC v143, C++ CMake tools, Windows 11 SDK |
| Windows SDK | 10.0.22621.0+ | For WinRT OCR, HID headers |
| CMake | 3.25+ | Bundled with VS Build Tools or standalone |
| .NET SDK | 8.0+ | For WinUI 3 shell |
| Node.js | 18+ | For browser extension TypeScript build |
| WiX Toolset v4 | 4.x | Optional — for MSI installer only |

All C++ dependencies are either vendored (`third_party/`) or fetched automatically by CMake (`FetchContent`):
- **SQLite 3.45.2** — vendored at `third_party/sqlite/`
- **GoogleTest 1.14.0** — fetched by CMake on first configure

---

## Quick start

```powershell
# 1. Configure (from repository root)
cmake -B build -G "Visual Studio 17 2022" -A x64

# 2. Build Debug
cmake --build build --config Debug

# 3. Run unit tests
.\build\bin\Debug\AccessOSUnitTests.exe

# 4. Build WinUI 3 shell
cd src/UI
dotnet build AccessOS.UI.csproj -c Debug
```

---

## Build targets

| CMake Target | Output | Description |
|---|---|---|
| `AccessOSCore` | `build/lib/Debug/AccessOSCore.lib` | Static library — all subsystems |
| `AccessOSCoreDll` | `build/bin/Debug/AccessOSCore_d.dll` | DLL P/Invoke surface (38 C exports) |
| `AccessOSUnitTests` | `build/bin/Debug/AccessOSUnitTests.exe` | GoogleTest runner |
| `AccessOS` | `build/bin/Debug/AccessOS.exe` | Diagnostic executable |

In Release, the DLL is named `AccessOSCore.dll` (without `_d`).

---

## Running tests

```powershell
# All tests
.\build\bin\Debug\AccessOSUnitTests.exe

# Filter to a subsystem
.\build\bin\Debug\AccessOSUnitTests.exe --gtest_filter="BrailleSubsystem*"

# XML output (for CI)
.\build\bin\Debug\AccessOSUnitTests.exe --gtest_output="xml:results.xml"

# Via CTest
ctest --test-dir build --build-config Debug --output-on-failure
```

Expected: **804 tests, 0 failures**.

---

## Building the browser extension

```powershell
cd src/BrowserExtension
npm install
npx tsc
# Output: dist/
```

Load the `dist/` folder as an unpacked extension in Chrome/Edge (Developer mode).

---

## Building the installer

```powershell
# MSIX only (no WiX required)
.\scripts\build-installer.ps1 -Config Release

# MSIX + MSI (requires: dotnet tool install --global wix)
.\scripts\build-installer.ps1 -Config Release -BuildMsi

# Output written to .\dist\
```

---

## Release build verification checklist

```powershell
# 1. Release C++ build
cmake --build build --config Release --target AccessOSCore AccessOSCoreDll AccessOSUnitTests

# 2. Release tests (must be 804/804)
.\build\bin\Release\AccessOSUnitTests.exe

# 3. DLL export count (must be 38)
dumpbin /exports .\build\bin\Release\AccessOSCore.dll | Select-String "Acos" | Measure-Object

# 4. WinUI 3 Release build
cd src/UI
dotnet build AccessOS.UI.csproj -c Release
```

---

## Known quirks

| Issue | Resolution |
|---|---|
| `UIAutomationCore.h` redefinition errors | Use `UIAIncludes.h` canonical include guard |
| `sphelper.h` requires ATL | Use direct COM enumeration instead |
| `SpeechManager.cpp` needs `<objbase.h>` | Explicit include in that file |
| `extern "C"` + `ACOS_API` conflict | `AccessOSExports.cpp` deliberately does NOT include `AccessOSExports.h` |
| LNK4098 LIBCMT warning | Pre-existing; suppressed via `/NODEFAULTLIB:LIBCMT` if needed |
| Release DLL import lib name collision | Fixed via `ARCHIVE_OUTPUT_NAME_RELEASE = AccessOSCoreDll` |
| WinRT `OcrResult` name collision | Use `WinOcr::` / `WinImaging::` / `WinFoundCol::` namespace aliases |
| `PublishTrimmed` firing during `dotnet build` | Gated on `$(PublishProtocol) != ''` in csproj |

---

## Project structure

```
AccessOS/
├── src/
│   ├── Core/               C++ core (STATIC + SHARED targets)
│   │   ├── Accessibility/  UIA provider
│   │   ├── Adapters/       UiaAdapter, BrowserAdapter, AdapterRegistry
│   │   ├── Announcement/   AnnouncementEngine
│   │   ├── Audio/          EarconManager, WaveEarconEngine
│   │   ├── Braille/        BrailleTranslator, BrailleDisplayManager, HidBrailleDisplay
│   │   ├── Bridge/         AccessOSExports (C API / DLL surface)
│   │   ├── Browse/         VirtualDocument, VirtualCursor, VirtualNode
│   │   ├── Browser/        NativeMessagingHost
│   │   ├── Commands/       CommandRegistry, ShortcutManager, KeyboardManager
│   │   ├── Context/        ContextDetector, ContextEngine
│   │   ├── Diagnostics/    DiagnosticsManager, FileSink
│   │   ├── Error/          AccessError, Result<T>
│   │   ├── Events/         EventEngine, EventQueue, UIAEventSink
│   │   ├── Focus/          FocusManager
│   │   ├── I18n/           LocaleManager (EN/FR/DE)
│   │   ├── Logging/        Logger
│   │   ├── Navigation/     NavigationEngine
│   │   ├── OCR/            OcrManager, WinRtOcrEngine
│   │   ├── Privacy/        PrivacyFilter
│   │   ├── Reader/         AccessReader, SayAll, ClipboardReader
│   │   ├── Rules/          AppRuleManager
│   │   ├── Semantic/       SemanticCache, SemanticNormalizer, AccessNode
│   │   ├── Settings/       SettingsManager, SqliteSettingsStore
│   │   ├── Speech/         SpeechManager, SapiSpeechEngine, VoiceProfile
│   │   └── Table/          TableNavigator
│   ├── UI/                 WinUI 3 shell (C# / .NET 8)
│   └── BrowserExtension/   MV3 TypeScript extension
├── Tests/Unit/             GoogleTest test suite (804 tests)
├── installer/              WiX v4 MSI source + License.rtf
├── scripts/                build-installer.ps1
├── docs/                   Architecture, developer, user, privacy docs
├── third_party/sqlite/     SQLite 3.45.2 amalgamation
├── .github/workflows/      CI/CD pipeline (4 jobs)
├── CMakeLists.txt          Root CMake
└── CHANGELOG.md
```
