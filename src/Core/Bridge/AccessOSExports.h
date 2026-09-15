// AccessOS/src/Core/Bridge/AccessOSExports.h
//
// Thin extern "C" export surface for the AccessOS core library.
//
// Why: The WinUI 3 shell (C#) and any future language binding need a stable,
//      ABI-safe interface. C exports with primitive types (int, char*, double)
//      cross the managed/unmanaged boundary without COM or C++/CLI overhead.
//
// Rules:
//   - All functions are __cdecl with C linkage.
//   - Strings passed IN are null-terminated UTF-8 const char*.
//   - Strings passed OUT are written into caller-supplied buffers.
//   - No C++ exceptions cross this boundary — errors return int (0=ok).
//   - No COM interfaces cross this boundary.
//   - Thread-safe: callers may call from any thread.
//
// Handle convention:
//   ACOS_HANDLE is an opaque void* to a heap-allocated Core instance.
//   Create one with AcosCreate(), destroy with AcosDestroy().
//   Pass it to all subsequent calls.

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
  #ifdef ACCESSOS_BUILDING_DLL
    #define ACOS_API __declspec(dllexport) __cdecl
  #else
    #define ACOS_API __declspec(dllimport) __cdecl
  #endif
#else
  #define ACOS_API
#endif

typedef void* ACOS_HANDLE;

// ── Lifecycle ──────────────────────────────────────────────────────────────────

/// Create and initialize a new AccessOS core instance.
/// Returns non-null handle on success, nullptr on failure.
ACOS_API ACOS_HANDLE AcosCreate(void);

/// Shut down and free an AccessOS core instance.
ACOS_API void AcosDestroy(ACOS_HANDLE handle);

/// Returns 1 if the engine is running, 0 otherwise.
ACOS_API int AcosIsRunning(ACOS_HANDLE handle);

// ── Speech ────────────────────────────────────────────────────────────────────

/// Speak a UTF-8 text string at the given priority (0=Low,1=Normal,2=High,3=Critical).
/// Returns 0 on success, non-zero on error.
ACOS_API int AcosSpeakText(ACOS_HANDLE handle,
                            const char* text,
                            int         priority,
                            int         cancelPrevious);

/// Stop all speech immediately.
ACOS_API int AcosSpeechStop(ACOS_HANDLE handle);

/// Set speech rate. rate is in the range -10 (slowest) to +10 (fastest), 0 = default.
ACOS_API int AcosSpeechSetRate(ACOS_HANDLE handle, int rate);

/// Set speech volume. volume is in the range 0–100.
ACOS_API int AcosSpeechSetVolume(ACOS_HANDLE handle, int volume);

/// Set the active voice by its token ID (UTF-8 string).
ACOS_API int AcosSpeechSetVoice(ACOS_HANDLE handle, const char* voiceId);

// ── Focused element readout ───────────────────────────────────────────────────

/// Write the name of the currently focused element into bufOut (UTF-8, null-terminated).
/// bufSize is the caller-allocated buffer size in bytes.
/// Returns 0 on success, non-zero if no focused element or buffer too small.
ACOS_API int AcosGetFocusedName(ACOS_HANDLE handle,
                                 char*       bufOut,
                                 int         bufSize);

/// Write the role string of the focused element (e.g. "Button") into bufOut.
ACOS_API int AcosGetFocusedRole(ACOS_HANDLE handle,
                                 char*       bufOut,
                                 int         bufSize);

// ── Browser event injection ───────────────────────────────────────────────────

/// Inject a focus-changed event from the browser extension.
/// name, role are UTF-8 null-terminated strings.
/// Returns 0 on success.
ACOS_API int AcosInjectBrowserFocus(ACOS_HANDLE handle,
                                     const char* name,
                                     const char* role,
                                     const char* pageUrl);

/// Inject a page-loaded event.
ACOS_API int AcosInjectBrowserPageLoad(ACOS_HANDLE handle,
                                        const char* url,
                                        const char* title);

// ── Reader control ────────────────────────────────────────────────────────────

/// Re-read (re-announce) the currently focused element.
/// Returns 0 on success, -1 if not running.
ACOS_API int AcosReadFocused(ACOS_HANDLE handle);

/// Set verbosity level (0=Minimal, 1=Standard, 2=Detailed, 3=Developer).
/// Returns 0 on success.
ACOS_API int AcosSetVerbosity(ACOS_HANDLE handle, int level);

/// Write the current application context type string into bufOut.
/// e.g. "Browser", "Terminal", "CodeEditor", "Unknown".
/// Returns 0 on success.
ACOS_API int AcosGetContextType(ACOS_HANDLE handle,
                                 char*       bufOut,
                                 int         bufSize);

// ── Diagnostics ───────────────────────────────────────────────────────────────

/// Fill caller-supplied uint64_t slots with runtime counters.
/// Any pointer may be NULL — that counter is skipped.
/// Returns 0 on success.
ACOS_API int AcosGetDiagnostics(ACOS_HANDLE handle,
                                 unsigned long long* outEventsProcessed,
                                 unsigned long long* outFocusChanges,
                                 unsigned long long* outSpeechUtterances,
                                 unsigned long long* outContextSwitches,
                                 unsigned long long* outUptimeMs);

/// Direct all logger output to a UTF-8 file path.
/// Returns 0 on success, -2 if the file cannot be opened.
ACOS_API int AcosSetLogFile(ACOS_HANDLE handle, const char* path);

/// Reset all runtime counters to zero.
/// Returns 0 on success.
ACOS_API int AcosResetCounters(ACOS_HANDLE handle);

// ── Settings ──────────────────────────────────────────────────────────────────

/// Read a raw settings key into bufOut. Returns -2 if key not found.
ACOS_API int AcosSettingsGet(ACOS_HANDLE handle,
                              const char* key,
                              char*       bufOut,
                              int         bufSize);

/// Write a raw settings key/value. Returns 0 on success.
ACOS_API int AcosSettingsSet(ACOS_HANDLE handle,
                              const char* key,
                              const char* value);

/// Persist current runtime state back to the settings database.
/// Returns 0 on success, -1 if settings are unavailable.
ACOS_API int AcosSettingsSave(ACOS_HANDLE handle);

// ── Say All (ACCESSOS-031) ────────────────────────────────────────────────────

/// Start continuous reading from current browse cursor position (background thread).
/// Returns 0 on success, -1 if browse mode is not active.
ACOS_API int AcosSayAll(ACOS_HANDLE handle);

/// Stop Say All.
ACOS_API int AcosSayAllStop(ACOS_HANDLE handle);

/// Returns 1 if Say All is currently running.
ACOS_API int AcosSayAllIsRunning(ACOS_HANDLE handle);

// ── Typing Echo (ACCESSOS-032) ────────────────────────────────────────────────

/// Set typing echo mode: 0=Off, 1=Char, 2=Word, 3=Both.
ACOS_API int AcosSetTypingEcho(ACOS_HANDLE handle, int mode);

/// Get current typing echo mode.
ACOS_API int AcosGetTypingEcho(ACOS_HANDLE handle);

// ── Clipboard Reading (ACCESSOS-033) ─────────────────────────────────────────

/// Speak clipboard text. Returns 0=success, 1=empty, -1=error.
ACOS_API int AcosReadClipboard(ACOS_HANDLE handle);

// ── Browse Mode ───────────────────────────────────────────────────────────────

/// Toggle browse mode. Returns 1 if now active, 0 if inactive, -1 on error.
ACOS_API int AcosBrowseToggle(ACOS_HANDLE handle);

/// Returns 1 if browse mode is active, 0 otherwise.
ACOS_API int AcosBrowseIsActive(ACOS_HANDLE handle);

/// Move virtual cursor to the next element. Returns 0=ok, 1=boundary, -1=error.
ACOS_API int AcosBrowseMoveNext(ACOS_HANDLE handle);

/// Move virtual cursor to the previous element.
ACOS_API int AcosBrowseMovePrev(ACOS_HANDLE handle);

/// Move to the next heading.
ACOS_API int AcosBrowseMoveNextHeading(ACOS_HANDLE handle);

/// Move to the previous heading.
ACOS_API int AcosBrowseMovePrevHeading(ACOS_HANDLE handle);

// ── Table Reading ─────────────────────────────────────────────────────────────

/// Write current table cell announcement into bufOut.
/// Returns 0 on success, -1 if no active table, -2 if buffer too small.
ACOS_API int AcosTableGetCell(ACOS_HANDLE handle, char* bufOut, int bufSize);

/// Move to next table cell, write announcement into bufOut.
ACOS_API int AcosTableMoveNext(ACOS_HANDLE handle, char* bufOut, int bufSize);

/// Move to previous table cell, write announcement into bufOut.
ACOS_API int AcosTableMovePrev(ACOS_HANDLE handle, char* bufOut, int bufSize);

/// Move to next table row, write announcement into bufOut.
ACOS_API int AcosTableMoveNextRow(ACOS_HANDLE handle, char* bufOut, int bufSize);

/// Move to previous table row, write announcement into bufOut.
ACOS_API int AcosTableMovePrevRow(ACOS_HANDLE handle, char* bufOut, int bufSize);

#ifdef __cplusplus
} // extern "C"
#endif
