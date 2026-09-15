# AccessOS User Guide

**Version 1.0** · Windows 10 (1809) and later

---

## Table of Contents

1. [What is AccessOS?](#what-is-accessos)
2. [System Requirements](#system-requirements)
3. [Installation](#installation)
4. [Getting Started](#getting-started)
5. [Keyboard Commands](#keyboard-commands)
6. [Browse Mode](#browse-mode)
7. [Table Reading](#table-reading)
8. [Speech Settings](#speech-settings)
9. [Audio Cues (Earcons)](#audio-cues-earcons)
10. [Browse Mode Settings](#browse-mode-settings)
11. [System Tray Icon](#system-tray-icon)
12. [Auto-Start with Windows](#auto-start-with-windows)
13. [Per-Application Rules](#per-application-rules)
14. [Voice Profiles](#voice-profiles)
15. [Privacy](#privacy)
16. [Troubleshooting](#troubleshooting)
17. [Keyboard Shortcut Reference](#keyboard-shortcut-reference)

---

## What is AccessOS?

AccessOS is a native Windows screen reader designed for keyboard users who rely on audio descriptions of the screen to navigate their computer. It reads aloud the names, roles, and states of interface elements as you move focus, announces application context changes, provides a virtual cursor for reading web pages and documents, and supports refreshable Braille displays.

AccessOS works entirely offline — it does not send data to the internet.

---

## System Requirements

| Requirement | Minimum |
|---|---|
| Operating System | Windows 10 version 1809 (build 17763) |
| Architecture | x64 |
| RAM | 256 MB available |
| Storage | 50 MB |
| Audio | Any audio output device |
| Speech | Windows built-in text-to-speech (SAPI) |

---

## Installation

### MSIX Package (recommended)

1. Download `AccessOS-Setup.msix` from the [Releases](https://github.com/accessos/AccessOS/releases) page.
2. Double-click the downloaded file.
3. Click **Install** in the installer dialog.
4. AccessOS appears in the Start Menu under **A**.

### Traditional MSI

1. Download `AccessOS-Setup.msi`.
2. Run `msiexec /i AccessOS-Setup.msi` or double-click the file.
3. Follow the installation wizard.

### Building from source

See [docs/developer/BUILDING.md](../developer/BUILDING.md).

---

## Getting Started

1. Launch **AccessOS** from the Start Menu or desktop shortcut.
2. Click **Start** in the control panel to begin listening.
3. Press **Tab** or **Arrow Keys** in any application — AccessOS announces each focused element.
4. Press **CapsLock** as the modifier key for all AccessOS commands (similar to the NVDA key).

### Stopping AccessOS

- Click **Stop** in the control panel, or
- Press **CapsLock + F4**, or
- Right-click the tray icon and choose **Exit**.

---

## Keyboard Commands

The **CapsLock** key is the AccessOS modifier key. Hold **CapsLock** and press a command key.

| Command | Keys |
|---|---|
| Read focused element | CapsLock + Enter |
| Read all from here | CapsLock + A |
| Stop speech | CapsLock + Ctrl + S |
| Read window title | CapsLock + T |
| Move to next element | CapsLock + Right Arrow |
| Move to previous element | CapsLock + Left Arrow |
| Next heading | CapsLock + H |
| Previous heading | CapsLock + Shift + H |
| Next link | CapsLock + K |
| Previous link | CapsLock + Shift + K |
| Next button | CapsLock + B |
| Previous button | CapsLock + Shift + B |
| Next form field | CapsLock + F |
| Previous form field | CapsLock + Shift + F |
| Toggle browse mode | CapsLock + Space |
| Read clipboard | CapsLock + C |
| Open AccessOS settings | CapsLock + F1 |
| Exit AccessOS | CapsLock + F4 |

---

## Browse Mode

Browse Mode lets you read through a document or web page as if it were plain text, using keyboard navigation keys without moving the actual application focus.

### Entering Browse Mode

- Press **CapsLock + Space** to toggle Browse Mode on or off.
- AccessOS announces "Browse mode on" or "Browse mode off".
- When entering a browser or document application, Browse Mode activates automatically if that option is enabled in settings.

### Navigating in Browse Mode

| Key | Action |
|---|---|
| Down Arrow | Next element |
| Up Arrow | Previous element |
| H | Next heading |
| Shift + H | Previous heading |
| K | Next link |
| Shift + K | Previous link |
| F | Next form field |
| Shift + F | Previous form field |
| T | Next table |
| Escape | Exit browse mode |

---

## Table Reading

When focus is inside a table or grid, AccessOS announces cell coordinates and header information.

### Navigation

| Key | Action |
|---|---|
| CapsLock + Right | Move to next column |
| CapsLock + Left | Move to previous column |
| CapsLock + Down | Move to next row |
| CapsLock + Up | Move to previous row |

AccessOS announces: **"Cell content, row N, column M"** along with any row or column headers detected.

### Header Announcement Modes

- **None** — only cell content is announced.
- **Row** — row header is announced with each cell.
- **Column** — column header is announced with each cell.
- **Both** — row and column headers announced (default).

---

## Speech Settings

Access speech settings in the AccessOS control panel under **Speech Settings**.

| Setting | Description |
|---|---|
| **Voice** | Choose from any installed SAPI voice. |
| **Verbosity** | Minimal / Standard / Detailed / Developer |
| **Speech Rate** | Adjust speaking speed (slider: 0.5× to 3.0×) |
| **Volume** | Adjust speech volume (0–100%) |

### Verbosity levels

- **Minimal** — name and role only.
- **Standard** — name, role, state, and value.
- **Detailed** — all of the above plus position information.
- **Developer** — all properties including raw UIA element IDs.

---

## Audio Cues (Earcons)

AccessOS plays short tones to indicate important events without using speech:

| Earcon | Event |
|---|---|
| Rising tone | Focus changed to an interactive element |
| Error chord | An error or alert is shown |
| Click | A link was focused |
| Soft click | A button was focused |

### Earcon settings

Open **Audio & Earcons** in the control panel to:
- Enable or disable earcons globally.
- Adjust earcon volume independently of speech volume.
- Enable or disable individual earcon types.

---

## Browse Mode Settings

Open **Browse Mode** in the control panel to configure:

| Setting | Description |
|---|---|
| **Enable Browse Mode** | Manually toggle browse mode on or off. |
| **Auto-detect documents** | Automatically enter browse mode when a document or browser is focused. |
| **Reading unit** | Choose what is announced as you navigate: Character, Word, Sentence, or Paragraph. |

---

## System Tray Icon

After AccessOS starts, a tray icon appears in the Windows notification area (bottom-right corner of the taskbar):

- **Double-click** the tray icon to bring the AccessOS control panel to the front.
- **Right-click** the tray icon to open a context menu with **Open AccessOS** and **Exit** options.

---

## Auto-Start with Windows

To have AccessOS start automatically when you log in to Windows:

1. Open the AccessOS control panel.
2. Scroll to the **System** section.
3. Enable **Start AccessOS with Windows**.

This writes a registry entry under `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` (current user only — no administrator rights required).

---

## Per-Application Rules

You can customise AccessOS behaviour for specific applications (e.g. set a different verbosity in a game, or disable earcons in a video player).

Per-application rules are configured via the Settings API. Each rule is keyed to the application's `.exe` filename:

```
apprule.notepad.exe.verbosity = 0
apprule.notepad.exe.browse_auto = 0
apprule.vlc.exe.earcons = 0
```

Rules are stored in the AccessOS settings database (`%APPDATA%\AccessOS\settings.db`).

---

## Voice Profiles

A Voice Profile saves your speech settings (voice, rate, volume) as a named preset.

| Profile | Suggested use |
|---|---|
| **Default** | Normal daily use |
| **FastRead** | High-speed browsing; high rate, reduced verbosity |
| **Quiet** | Low-volume output for shared spaces |

Profiles are managed via the Settings API or programmatically via the C++ `VoiceProfileManager`.

---

## Privacy

AccessOS is designed with privacy as a core principle:

- **No network requests** — AccessOS never communicates with external servers.
- **No logging of sensitive data** — passwords, PINs, credit card numbers, and authentication tokens are never read aloud or logged, even when typed.
- **Local storage only** — settings are stored in `%APPDATA%\AccessOS\settings.db` on your local machine.
- **No telemetry** — usage data is never collected.

The full privacy policy is in [docs/privacy/PRIVACY.md](../privacy/PRIVACY.md).

---

## Troubleshooting

### AccessOS doesn't speak when I press Start

1. Check that your audio device is connected and working.
2. Ensure at least one SAPI voice is installed: **Settings → Time & Language → Speech → Manage voices**.
3. Verify that `AccessOSCore.dll` is present in the same directory as `AccessOS.UI.exe`.

### Speech cuts out or is garbled

- Reduce the speech rate in **Speech Settings**.
- Try a different SAPI voice.

### The tray icon does not appear

- The tray icon requires the taskbar notification area to be visible. If it is hidden, expand the notification area overflow to find it.

### AccessOS does not read in a specific application

- Some applications use non-standard UI frameworks that may require custom per-application rules.
- Check that the application uses accessible UI (UIA-compatible).
- File an issue on the [GitHub repository](https://github.com/accessos/AccessOS/issues).

### The browser extension is not working

- Ensure the native messaging host is registered. Run the AccessOS installer or manually run `scripts/register-native-host.ps1`.
- The extension works with Chromium-based browsers (Chrome, Edge, Brave) and Firefox.

---

## Keyboard Shortcut Reference

A full printable shortcut reference card is available at:

```
docs/user/ShortcutCard.md
```

Or press **CapsLock + F1** inside any application to hear the shortcuts announced.

---

*AccessOS is open-source software released under the MIT License.*  
*Documentation last updated: 2025*
