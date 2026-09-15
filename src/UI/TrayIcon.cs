// TrayIcon.cs — Win32 system tray icon for AccessOS (ACCESSOS-037)
//
// Creates a notification area (system tray) icon using Shell_NotifyIconW.
// The icon shows a context menu with Open / Exit entries and fires
// the OnOpen / OnExit callbacks when those items are clicked.
//
// Usage:
//   var tray = new TrayIcon(hwnd);
//   tray.OnOpen += () => window.Activate();
//   tray.OnExit += () => Application.Current.Exit();
//   tray.Create("AccessOS", iconHandle);
//   // ... app running ...
//   tray.Dispose();

using System;
using System.Runtime.InteropServices;
using System.Threading;

namespace AccessOS_UI;

internal sealed class TrayIcon : IDisposable
{
    // ── Win32 constants ──────────────────────────────────────────────────────
    private const int WM_APP_TRAY     = 0x8001;   // custom WM_ for tray callbacks
    private const int WM_LBUTTONDBLCLK = 0x0203;
    private const int WM_RBUTTONUP    = 0x0205;
    private const int NIN_SELECT      = 0x0400;
    private const uint NIF_MESSAGE    = 0x00000001;
    private const uint NIF_ICON       = 0x00000002;
    private const uint NIF_TIP        = 0x00000004;
    private const uint NIM_ADD        = 0x00000000;
    private const uint NIM_MODIFY     = 0x00000001;
    private const uint NIM_DELETE     = 0x00000002;
    private const uint NIM_SETVERSION = 0x00000004;
    private const uint NOTIFYICON_VERSION_4 = 4;
    private const uint TPM_RIGHTBUTTON = 0x0002;
    private const uint TPM_RETURNCMD   = 0x0100;

    private const int MF_STRING    = 0x0000;
    private const int MF_SEPARATOR = 0x0800;

    private const int IDM_OPEN = 1001;
    private const int IDM_EXIT = 1002;

    // ── P/Invoke ─────────────────────────────────────────────────────────────
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    private struct NOTIFYICONDATA
    {
        public uint cbSize;
        public nint hWnd;
        public uint uID;
        public uint uFlags;
        public uint uCallbackMessage;
        public nint hIcon;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
        public string szTip;
        public uint dwState;
        public uint dwStateMask;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
        public string szInfo;
        public uint uTimeoutOrVersion;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 64)]
        public string szInfoTitle;
        public uint dwInfoFlags;
        public Guid guidItem;
        public nint hBalloonIcon;
    }

    [DllImport("shell32.dll", CharSet = CharSet.Unicode)]
    private static extern bool Shell_NotifyIconW(uint dwMessage, ref NOTIFYICONDATA lpdata);

    [DllImport("user32.dll")]
    private static extern nint CreatePopupMenu();

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    private static extern bool AppendMenuW(nint hMenu, uint uFlags, nint uIDNewItem, string lpNewItem);

    [DllImport("user32.dll")]
    private static extern bool DestroyMenu(nint hMenu);

    [DllImport("user32.dll")]
    private static extern uint TrackPopupMenu(nint hMenu, uint uFlags, int x, int y,
                                               int nReserved, nint hWnd, nint prcRect);

    [DllImport("user32.dll")]
    private static extern bool GetCursorPos(out POINT lpPoint);

    [DllImport("user32.dll")]
    private static extern bool SetForegroundWindow(nint hWnd);

    [StructLayout(LayoutKind.Sequential)]
    private struct POINT { public int X; public int Y; }

    [DllImport("user32.dll")]
    private static extern nint LoadIconW(nint hInstance, nint lpIconName);
    private static readonly nint IDI_APPLICATION = new nint(32512);

    // ── state ────────────────────────────────────────────────────────────────
    private readonly nint   _hwnd;
    private bool            _created;
    private nint            _hIcon;

    public event Action? OnOpen;
    public event Action? OnExit;

    // ── construction ─────────────────────────────────────────────────────────
    public TrayIcon(nint hwnd)
    {
        _hwnd = hwnd;
    }

    /// <summary>
    /// Creates the tray icon. Call after the window handle is valid.
    /// </summary>
    /// <param name="tooltip">Tooltip text shown on hover (max 127 chars).</param>
    /// <param name="hIcon">Optional icon handle. If zero, uses the default application icon.</param>
    public void Create(string tooltip, nint hIcon = 0)
    {
        if (_created) return;

        _hIcon = hIcon != 0 ? hIcon
                             : LoadIconW(0, IDI_APPLICATION);

        var nid = BuildNid();
        nid.uFlags          = NIF_MESSAGE | NIF_ICON | NIF_TIP;
        nid.uCallbackMessage= WM_APP_TRAY;
        nid.hIcon           = _hIcon;
        nid.szTip           = tooltip.Length > 127 ? tooltip[..127] : tooltip;

        Shell_NotifyIconW(NIM_ADD, ref nid);

        // Enable version 4 to get LOWORD(lParam) = notification code
        nid.uFlags              = 0;
        nid.uTimeoutOrVersion   = NOTIFYICON_VERSION_4;
        Shell_NotifyIconW(NIM_SETVERSION, ref nid);

        _created = true;
    }

    /// <summary>
    /// Update the tooltip text on the existing icon.
    /// </summary>
    public void SetTooltip(string tooltip)
    {
        if (!_created) return;
        var nid = BuildNid();
        nid.uFlags = NIF_TIP;
        nid.szTip  = tooltip.Length > 127 ? tooltip[..127] : tooltip;
        Shell_NotifyIconW(NIM_MODIFY, ref nid);
    }

    /// <summary>
    /// Handle a Win32 message forwarded from the window's WndProc.
    /// Returns true if the message was consumed.
    /// </summary>
    public bool HandleMessage(uint msg, nint wParam, nint lParam)
    {
        if (msg != WM_APP_TRAY) return false;

        uint notification = (uint)(lParam.ToInt64() & 0xFFFF);

        switch (notification)
        {
            case WM_LBUTTONDBLCLK:
            case NIN_SELECT:
                OnOpen?.Invoke();
                return true;

            case WM_RBUTTONUP:
                ShowContextMenu();
                return true;
        }
        return false;
    }

    // ── private helpers ───────────────────────────────────────────────────────
    private void ShowContextMenu()
    {
        nint hMenu = CreatePopupMenu();
        if (hMenu == 0) return;

        AppendMenuW(hMenu, MF_STRING,    IDM_OPEN, "Open AccessOS");
        AppendMenuW(hMenu, MF_SEPARATOR, 0,        string.Empty);
        AppendMenuW(hMenu, MF_STRING,    IDM_EXIT, "Exit");

        SetForegroundWindow(_hwnd);
        GetCursorPos(out var pt);

        uint cmd = TrackPopupMenu(hMenu,
                                   TPM_RIGHTBUTTON | TPM_RETURNCMD,
                                   pt.X, pt.Y, 0, _hwnd, 0);
        DestroyMenu(hMenu);

        if (cmd == IDM_OPEN) OnOpen?.Invoke();
        else if (cmd == IDM_EXIT) OnExit?.Invoke();
    }

    private NOTIFYICONDATA BuildNid() => new NOTIFYICONDATA
    {
        cbSize = (uint)Marshal.SizeOf<NOTIFYICONDATA>(),
        hWnd   = _hwnd,
        uID    = 1,
    };

    // ── IDisposable ───────────────────────────────────────────────────────────
    public void Dispose()
    {
        if (_created)
        {
            var nid = BuildNid();
            Shell_NotifyIconW(NIM_DELETE, ref nid);
            _created = false;
        }
    }
}
