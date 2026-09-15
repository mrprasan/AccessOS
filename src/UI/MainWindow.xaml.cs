// AccessOS/src/UI/MainWindow.xaml.cs

using Microsoft.UI.Xaml;

namespace AccessOS_UI;

/// <summary>
/// Shell window. Hosts the WinUI TitleBar and a root Frame.
/// All application content and logic lives in MainPage.
/// </summary>
public sealed partial class MainWindow : Window
{
    public MainWindow()
    {
        InitializeComponent();

        ExtendsContentIntoTitleBar = true;
        SetTitleBar(AppTitleBar);

        AppWindow.SetIcon("Assets/AppIcon.ico");
        AppWindow.Resize(new Windows.Graphics.SizeInt32(900, 620));

        RootFrame.Navigate(typeof(MainPage));
    }
}
