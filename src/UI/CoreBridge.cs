// AccessOS/src/UI/CoreBridge.cs
//
// P/Invoke bridge between the WinUI 3 shell and the AccessOS C++ core.
//
// Why: The C++ core (AccessOSCore.lib / AccessOSCore.dll) exposes a flat C API
//      declared in AccessOSExports.h. This class wraps every export with a
//      type-safe C# method, handles marshalling, and converts error codes into
//      exceptions or Result-style returns so callers never see raw integers.
//
// Threading:
//   - AcosCreate / AcosDestroy must be called from the UI thread.
//   - Speech methods are thread-safe (the C++ side queues requests).
//   - PropertyChanged events are dispatched on the UI thread via
//     DispatcherQueue.TryEnqueue.
//
// Lifecycle:
//   Call CoreBridge.Initialize() once at app startup.
//   Call CoreBridge.Shutdown()   once at app exit.

using System;
using System.Runtime.InteropServices;
using System.Text;
using Microsoft.UI.Dispatching;

namespace AccessOS_UI;

/// <summary>Snapshot of runtime diagnostic counters from the C++ core.</summary>
public sealed record DiagnosticsInfo(
    ulong EventsProcessed,
    ulong FocusChanges,
    ulong SpeechUtterances,
    ulong ContextSwitches,
    ulong UptimeMs);

/// <summary>
/// Wraps the AccessOS C++ core exported C API.
/// One instance per application lifetime.
/// </summary>
public sealed class CoreBridge : IDisposable
{
    // ── P/Invoke declarations ─────────────────────────────────────────────────

#if DEBUG
    private const string DllName = "AccessOSCore_d.dll";
#else
    private const string DllName = "AccessOSCore.dll";
#endif

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr AcosCreate();

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void AcosDestroy(IntPtr handle);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int AcosIsRunning(IntPtr handle);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl,
               CharSet = CharSet.Ansi)]
    private static extern int AcosSpeakText(IntPtr handle,
                                             [MarshalAs(UnmanagedType.LPStr)] string text,
                                             int priority,
                                             int cancelPrevious);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int AcosSpeechStop(IntPtr handle);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int AcosSpeechSetRate(IntPtr handle, int rate);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int AcosSpeechSetVolume(IntPtr handle, int volume);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl,
               CharSet = CharSet.Ansi)]
    private static extern int AcosSpeechSetVoice(IntPtr handle,
                                                  [MarshalAs(UnmanagedType.LPStr)] string voiceId);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int AcosGetFocusedName(IntPtr handle,
                                                  byte[] buf, int bufSize);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int AcosGetFocusedRole(IntPtr handle,
                                                  byte[] buf, int bufSize);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl,
               CharSet = CharSet.Ansi)]
    private static extern int AcosInjectBrowserFocus(IntPtr handle,
                                                      [MarshalAs(UnmanagedType.LPStr)] string name,
                                                      [MarshalAs(UnmanagedType.LPStr)] string role,
                                                      [MarshalAs(UnmanagedType.LPStr)] string pageUrl);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl,
               CharSet = CharSet.Ansi)]
    private static extern int AcosInjectBrowserPageLoad(IntPtr handle,
                                                         [MarshalAs(UnmanagedType.LPStr)] string url,
                                                         [MarshalAs(UnmanagedType.LPStr)] string title);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int AcosGetDiagnostics(IntPtr handle,
        out ulong eventsProcessed, out ulong focusChanges,
        out ulong speechUtterances, out ulong contextSwitches,
        out ulong uptimeMs);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl,
               CharSet = CharSet.Ansi)]
    private static extern int AcosSetLogFile(IntPtr handle,
        [MarshalAs(UnmanagedType.LPStr)] string path);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int AcosResetCounters(IntPtr handle);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl,
               CharSet = CharSet.Ansi)]
    private static extern int AcosSettingsGet(IntPtr handle,
        [MarshalAs(UnmanagedType.LPStr)] string key,
        byte[] bufOut, int bufSize);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl,
               CharSet = CharSet.Ansi)]
    private static extern int AcosSettingsSet(IntPtr handle,
        [MarshalAs(UnmanagedType.LPStr)] string key,
        [MarshalAs(UnmanagedType.LPStr)] string value);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int AcosSettingsSave(IntPtr handle);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int AcosReadFocused(IntPtr handle);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int AcosBrowseToggle(IntPtr handle);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int AcosBrowseIsActive(IntPtr handle);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int AcosSetVerbosity(IntPtr handle, int level);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int AcosGetContextType(IntPtr handle,
                                                  byte[] buf, int bufSize);

    // ── State ─────────────────────────────────────────────────────────────────

    private IntPtr _handle = IntPtr.Zero;
    private bool   _disposed;

    /// <summary>True when the native core is running.</summary>
    public bool IsRunning => _handle != IntPtr.Zero &&
                             AcosIsRunning(_handle) != 0;

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    /// <summary>
    /// Initialize the AccessOS native core.
    /// Throws <see cref="InvalidOperationException"/> if initialization fails.
    /// </summary>
    public void Initialize()
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        if (_handle != IntPtr.Zero) return;

        _handle = AcosCreate();
        if (_handle == IntPtr.Zero)
            throw new InvalidOperationException(
                "AcosCreate failed — native core could not be initialized. " +
                "Ensure AccessOSCore.dll is present and COM is available.");
    }

    /// <summary>Shut down the native core and release all resources.</summary>
    public void Shutdown()
    {
        if (_handle == IntPtr.Zero) return;
        AcosDestroy(_handle);
        _handle = IntPtr.Zero;
    }

    // ── IDisposable ───────────────────────────────────────────────────────────

    public void Dispose()
    {
        if (_disposed) return;
        Shutdown();
        _disposed = true;
    }

    // ── Speech ────────────────────────────────────────────────────────────────

    /// <summary>Speak a text string.</summary>
    /// <param name="text">UTF-8 text to speak.</param>
    /// <param name="priority">0=Low 1=Normal 2=High 3=Critical</param>
    /// <param name="cancelPrevious">True to cancel any in-progress speech.</param>
    public void SpeakText(string text, int priority = 1, bool cancelPrevious = false)
    {
        ThrowIfNotReady();
        AcosSpeakText(_handle, text, priority, cancelPrevious ? 1 : 0);
    }

    /// <summary>Stop all speech immediately.</summary>
    public void SpeechStop()
    {
        ThrowIfNotReady();
        AcosSpeechStop(_handle);
    }

    /// <summary>
    /// Set speech rate. -10 = slowest, 0 = default, +10 = fastest.
    /// Maps the UI slider (0.5–3.0) to the SAPI range (-10..+10).
    /// </summary>
    public void SetSpeechRate(double uiRate)
    {
        ThrowIfNotReady();
        // uiRate: 0.5=slowest, 1.0=normal, 3.0=fastest → map to -10..+10
        int sapiRate = (int)Math.Round((uiRate - 1.0) * 10.0);
        sapiRate = Math.Clamp(sapiRate, -10, 10);
        AcosSpeechSetRate(_handle, sapiRate);
    }

    /// <summary>Set speech volume 0–100.</summary>
    public void SetSpeechVolume(int volume)
    {
        ThrowIfNotReady();
        AcosSpeechSetVolume(_handle, Math.Clamp(volume, 0, 100));
    }

    /// <summary>Set active voice by display name or token ID.</summary>
    public void SetVoice(string voiceId)
    {
        ThrowIfNotReady();
        AcosSpeechSetVoice(_handle, voiceId);
    }

    // ── Focused element readout ───────────────────────────────────────────────

    /// <summary>
    /// Returns the name of the currently focused element, or null.
    /// </summary>
    public string? GetFocusedName()
    {
        if (_handle == IntPtr.Zero) return null;
        var buf = new byte[512];
        int rc = AcosGetFocusedName(_handle, buf, buf.Length);
        if (rc != 0) return null;
        return Encoding.UTF8.GetString(buf, 0,
            Array.IndexOf(buf, (byte)0) is int z && z >= 0 ? z : buf.Length);
    }

    /// <summary>
    /// Returns the role of the currently focused element, or null.
    /// </summary>
    public string? GetFocusedRole()
    {
        if (_handle == IntPtr.Zero) return null;
        var buf = new byte[64];
        int rc = AcosGetFocusedRole(_handle, buf, buf.Length);
        if (rc != 0) return null;
        return Encoding.UTF8.GetString(buf, 0,
            Array.IndexOf(buf, (byte)0) is int z && z >= 0 ? z : buf.Length);
    }

    // ── Browser event injection ───────────────────────────────────────────────

    /// <summary>Inject a browser focus-changed event from the extension bridge.</summary>
    public void InjectBrowserFocus(string name, string role, string pageUrl)
    {
        if (_handle == IntPtr.Zero) return;
        AcosInjectBrowserFocus(_handle, name, role, pageUrl);
    }

    /// <summary>Inject a browser page-loaded event.</summary>
    public void InjectBrowserPageLoad(string url, string title)
    {
        if (_handle == IntPtr.Zero) return;
        AcosInjectBrowserPageLoad(_handle, url, title);
    }

    // ── Reader control ────────────────────────────────────────────────────────

    /// <summary>
    /// Re-announce the currently focused element.
    /// </summary>
    public void ReadFocused()
    {
        if (_handle == IntPtr.Zero) return;
        AcosReadFocused(_handle);
    }

    // ── Browse Mode ───────────────────────────────────────────────────────────

    /// <summary>Toggle browse mode on/off.</summary>
    public void BrowseToggle()
    {
        if (_handle == IntPtr.Zero) return;
        AcosBrowseToggle(_handle);
    }

    /// <summary>Returns true if browse mode is currently active.</summary>
    public bool IsBrowseActive()
    {
        if (_handle == IntPtr.Zero) return false;
        return AcosBrowseIsActive(_handle) != 0;
    }

    /// <summary>
    /// Set the verbosity level for announcements.
    /// </summary>
    /// <param name="level">0=Minimal, 1=Standard, 2=Detailed, 3=Developer</param>
    public void SetVerbosity(int level)
    {
        if (_handle == IntPtr.Zero) return;
        AcosSetVerbosity(_handle, Math.Clamp(level, 0, 3));
    }

    /// <summary>
    /// Returns the current application context type string,
    /// e.g. "Browser", "Terminal", "CodeEditor", or null if not running.
    /// </summary>
    public string? GetContextType()
    {
        if (_handle == IntPtr.Zero) return null;
        var buf = new byte[64];
        int rc = AcosGetContextType(_handle, buf, buf.Length);
        if (rc != 0) return null;
        return Encoding.UTF8.GetString(buf, 0,
            Array.IndexOf(buf, (byte)0) is int z && z >= 0 ? z : buf.Length);
    }

    // ── Diagnostics ───────────────────────────────────────────────────────────

    /// <summary>
    /// Returns a snapshot of runtime counters, or null if not running.
    /// </summary>
    public DiagnosticsInfo? GetDiagnostics()
    {
        if (_handle == IntPtr.Zero) return null;
        int rc = AcosGetDiagnostics(_handle,
            out ulong ev, out ulong fc, out ulong sp, out ulong cs, out ulong up);
        if (rc != 0) return null;
        return new DiagnosticsInfo(ev, fc, sp, cs, up);
    }

    /// <summary>
    /// Direct the C++ logger to write to the specified file path.
    /// Returns true on success.
    /// </summary>
    public bool SetLogFile(string path)
    {
        if (_handle == IntPtr.Zero) return false;
        return AcosSetLogFile(_handle, path) == 0;
    }

    /// <summary>Reset all runtime counters to zero.</summary>
    public void ResetCounters()
    {
        if (_handle == IntPtr.Zero) return;
        AcosResetCounters(_handle);
    }

    // ── Settings ──────────────────────────────────────────────────────────────

    /// <summary>Read a raw settings key. Returns null if not found.</summary>
    public string? SettingsGet(string key)
    {
        if (_handle == IntPtr.Zero) return null;
        var buf = new byte[512];
        int rc = AcosSettingsGet(_handle, key, buf, buf.Length);
        if (rc != 0) return null;
        return Encoding.UTF8.GetString(buf, 0,
            Array.IndexOf(buf, (byte)0) is int z && z >= 0 ? z : buf.Length);
    }

    /// <summary>Write a raw settings key/value pair.</summary>
    public void SettingsSet(string key, string value)
    {
        if (_handle == IntPtr.Zero) return;
        AcosSettingsSet(_handle, key, value);
    }

    /// <summary>Convenience alias for SettingsSet — write and persist a key/value.</summary>
    public void SaveSetting(string key, string value)
    {
        if (_handle == IntPtr.Zero) return;
        AcosSettingsSet(_handle, key, value);
        AcosSettingsSave(_handle);
    }

    /// <summary>Persist current runtime state to the settings database.</summary>
    public void SettingsSave()
    {
        if (_handle == IntPtr.Zero) return;
        AcosSettingsSave(_handle);
    }

    // ── Helpers ───────────────────────────────────────────────────────────────

    private void ThrowIfNotReady()
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        if (_handle == IntPtr.Zero)
            throw new InvalidOperationException(
                "CoreBridge is not initialized. Call Initialize() first.");
    }
}
