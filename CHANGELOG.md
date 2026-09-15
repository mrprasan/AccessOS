# Changelog

All notable changes to AccessOS are documented here.  
Format: [Semantic Versioning](https://semver.org/). Dates are YYYY-MM-DD.

---

## [1.0.0] — 2025

### Added

#### Core accessibility pipeline
- **UI Automation acquisition** — focus, property-change, structure-change, and notification event handlers via `UIAEventSink` / `EventEngine`
- **Semantic model** — `SemanticCache`, `SemanticNormalizer`, `AccessNode` with full role/state/name/value/bounds
- **Focus management** — `FocusManager` tracks element history and fires focus-change events
- **Context engine** — `ContextDetector` classifies window context (Browser, Terminal, CodeEditor, Spreadsheet, …) via UIA window-class heuristics
- **Announcement engine** — `AnnouncementEngine` templates structured speech from semantic properties
- **AccessReader** — central pipeline hub; wires event engine → announcements → speech
- **Privacy filter** — suppresses speech for password fields, PINs, OTPs, credit card fields

#### Speech
- **SAPI speech engine** — wraps Windows Speech API; rate, volume, voice selection; queued delivery
- **Speech formatter** — constructs human-readable utterances from `AccessNode` snapshots
- **Say All** — continuous document reading via `SayAll` / `VirtualCursor`
- **Typing echo** — `Off / Char / Word / Both` modes; Protected field suppression
- **Voice profiles** — named presets (voice ID + rate + volume) persisted to SQLite

#### Browse Mode / Virtual Cursor
- **VirtualDocument** — flat node buffer built from UIA tree snapshots
- **VirtualCursor** — stateful cursor with element, heading, landmark, form-field, link navigation
- **Browse toggle** — `CapsLock + Space`; auto-detect for document/browser contexts

#### Table / Grid reading
- **TableNavigator** — (row, col) cursor; header-mode announcements (None / Row / Col / Both)
- **Table DLL exports** — `AcosTableGetCell`, `AcosTableMoveNext/Prev`, `AcosTableMoveNextRow/PrevRow`

#### Braille
- **NABCC/Grade-1 translator** — 8-dot cell encoding, Unicode Braille block
- **BrailleDisplayManager** — viewport panning, cursor routing, WriteText / WriteCells / Flush
- **HID Braille Display Driver** — Win32 SetupAPI device enumeration; `OVERLAPPED` async HID write; 8 known VID/PID entries (HumanWare, Freedom Scientific, HIMS, Papenmeier)

#### OCR
- **WinRT OCR engine** — `Windows.Media.Ocr.OcrEngine` via WRL (no C++/WinRT); async wait via Win32 event
- **OcrManager** — engine registry and dispatch
- **Screen region capture** — `CaptureScreenRect` via GDI `BitBlt`

#### Audio (Earcons)
- **WaveEarconEngine** — Win32 `waveOut` tone synthesis; 23 earcon IDs
- **EarconManager** — per-earcon enable, master enable/mute/volume; fires on focus/state changes

#### Settings
- **SQLite settings store** — vendored SQLite 3.45.2; `%APPDATA%\AccessOS\settings.db`
- **SettingsManager** — typed façade; speech, shortcuts, UI, privacy sections
- **Settings persistence integration** — `AcosSettingsGet/Set/Save` DLL exports

#### Diagnostics
- **DiagnosticsManager** — runtime counters (events, focus changes, speech utterances, context switches, uptime)
- **FileSink** — structured log output to file

#### Commands & Keyboard
- **CommandRegistry** — 15 built-in commands with descriptions
- **ShortcutManager** — VK code + modifier byte bindings, persisted to settings
- **KeyboardManager** — `CapsLock` (VK_CAPITAL) as AccessOS modifier; 12 default shortcuts

#### Native Messaging / Browser Extension
- **NativeMessagingHost** — JSON stdio pipe; Chrome/Edge/Firefox MV3 extension bridge
- **BrowserAdapter** — injects focus and page-load events from browser extension
- **Browser extension** — TypeScript/MV3 content script + background worker

#### WinUI 3 Shell
- **MainWindow / MainPage** — live C++ core via P/Invoke; 500 ms poll timer
- **Speech settings panel** — voice selector, verbosity, rate slider, volume slider
- **Browse Mode settings panel** — toggle, auto-detect, reading unit
- **Audio & Earcons settings panel** — master toggle, volume slider, per-earcon toggles
- **System settings panel** — auto-start toggle (HKCU registry)
- **Activity log** — scrolling event feed with clear button
- **System tray icon** — `Shell_NotifyIconW`; double-click = open, right-click = menu
- **Auto-start** — HKCU `Run` registry entry; no elevation required

#### Localisation / i18n
- **LocaleManager** — compile-time message catalogues; EN (complete), FR (partial), DE (partial)
- **`Msg()` / `MsgFmt()`** free functions for use throughout the core

#### Per-Application Rules
- **AppRuleManager** — per-exe rule sets: verbosity override, browse-mode auto, earcon toggle, role-template overrides; persisted to settings DB

#### Installer & CI/CD
- **WiX v4 MSI** — `installer/AccessOS.wxs`; per-machine install, shortcuts, Add/Remove Programs
- **MSIX** — single-project MSIX via `Package.appxmanifest` + `EnableMsixTooling`
- **Build script** — `scripts/build-installer.ps1` (MSIX + optional MSI)
- **GitHub Actions** — 4-job pipeline: C++ build+test → WinUI 3 build → MSIX package → GitHub Release

#### Documentation
- **User Guide** — `docs/user/UserGuide.md` (keyboard commands, browse mode, table reading, settings, privacy, troubleshooting)

### DLL exports (38 total)
`AcosCreate/Destroy/IsRunning` · `AcosSpeakText/SpeechStop/SetRate/SetVolume/SetVoice` · `AcosGetFocusedName/Role` · `AcosGetContextType` · `AcosReadFocused` · `AcosSetVerbosity` · `AcosGetDiagnostics/SetLogFile/ResetCounters` · `AcosSettingsGet/Set/Save` · `AcosInjectBrowserFocus/PageLoad` · `AcosBrowseToggle/IsActive/MoveNext/MovePrev/MoveNextHeading/MovePrevHeading` · `AcosTableGetCell/MoveNext/MovePrev/MoveNextRow/MovePrevRow` · `AcosSayAll/SayAllStop/SayAllIsRunning` · `AcosSetTypingEcho/GetTypingEcho` · `AcosReadClipboard`

### Test coverage
**804 unit tests** across 74 test suites — 100% pass rate in both Debug and Release.

---

## [Unreleased]

- Voice profile UI in WinUI 3 shell
- Per-application rules configuration UI
- Additional i18n locales (ES, PT, ZH, JA)
- HID braille display live hardware validation
- User-configurable keyboard shortcuts UI
