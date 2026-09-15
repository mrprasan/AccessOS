using Microsoft.UI.Xaml;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace AccessOS_UI;

/// <summary>
/// Provides application-specific behavior to supplement the default Application class.
/// </summary>
public partial class App : Application
{
    private MainWindow? _window;

    // Tray icon — kept alive for the lifetime of the app.
    internal TrayIcon? TrayIcon { get; private set; }

    /// <summary>
    /// Initializes the singleton application object.  This is the first line of authored code
    /// executed, and as such is the logical equivalent of main() or WinMain().
    /// </summary>
    public App()
    {
        InitializeComponent();
    }

    /// <summary>
    /// Invoked when the application is launched.
    /// </summary>
    /// <param name="args">Details about the launch request and process.</param>
    protected override void OnLaunched(Microsoft.UI.Xaml.LaunchActivatedEventArgs args)
    {
        _window = new MainWindow();
        _window.Activate();

        // Set up system tray icon after window is ready
        nint hwnd = WinRT.Interop.WindowNative.GetWindowHandle(_window);
        TrayIcon = new TrayIcon(hwnd);
        TrayIcon.OnOpen += () => _window.Activate();
        TrayIcon.OnExit += () => { TrayIcon.Dispose(); Exit(); };
        TrayIcon.Create("AccessOS – Screen Reader");
    }
}
