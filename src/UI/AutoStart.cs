// AutoStart.cs — Windows registry auto-start support (ACCESSOS-037)
//
// Reads and writes the HKCU\Software\Microsoft\Windows\CurrentVersion\Run key
// to enable or disable AccessOS launching at Windows startup.
// Uses HKCU (current user) — does NOT require elevation.

using System;
using Microsoft.Win32;

namespace AccessOS_UI;

internal static class AutoStart
{
    private const string RunKeyPath = @"Software\Microsoft\Windows\CurrentVersion\Run";
    private const string ValueName  = "AccessOS";

    /// <summary>
    /// Returns true if AccessOS is registered to run at Windows startup.
    /// </summary>
    public static bool IsEnabled()
    {
        try
        {
            using var key = Registry.CurrentUser.OpenSubKey(RunKeyPath, writable: false);
            if (key is null) return false;
            var val = key.GetValue(ValueName) as string;
            return !string.IsNullOrEmpty(val);
        }
        catch
        {
            return false;
        }
    }

    /// <summary>
    /// Enable auto-start: registers the current executable path in Run key.
    /// </summary>
    /// <returns>True on success.</returns>
    public static bool Enable()
    {
        try
        {
            string exePath = Environment.ProcessPath ?? string.Empty;
            if (string.IsNullOrEmpty(exePath)) return false;

            using var key = Registry.CurrentUser.OpenSubKey(RunKeyPath, writable: true)
                         ?? Registry.CurrentUser.CreateSubKey(RunKeyPath);
            if (key is null) return false;

            key.SetValue(ValueName, $"\"{exePath}\"", RegistryValueKind.String);
            return true;
        }
        catch
        {
            return false;
        }
    }

    /// <summary>
    /// Disable auto-start: removes the AccessOS entry from the Run key.
    /// </summary>
    /// <returns>True on success (including when the key didn't exist).</returns>
    public static bool Disable()
    {
        try
        {
            using var key = Registry.CurrentUser.OpenSubKey(RunKeyPath, writable: true);
            if (key is null) return true; // already absent
            key.DeleteValue(ValueName, throwOnMissingValue: false);
            return true;
        }
        catch
        {
            return false;
        }
    }

    /// <summary>
    /// Toggle auto-start state.
    /// </summary>
    /// <returns>New enabled state after toggle.</returns>
    public static bool Toggle()
    {
        if (IsEnabled())
        {
            Disable();
            return false;
        }
        else
        {
            Enable();
            return true;
        }
    }
}
