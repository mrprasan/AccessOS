// AccessOS/src/UI/ShortcutRow.xaml.cs

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace AccessOS_UI;

/// <summary>
/// A single shortcut row: key badge on the left, action label on the right.
/// </summary>
public sealed partial class ShortcutRow : UserControl
{
    // ── Dependency properties ─────────────────────────────────────────────────

    public static readonly DependencyProperty KeysProperty =
        DependencyProperty.Register(nameof(Keys), typeof(string),
            typeof(ShortcutRow),
            new PropertyMetadata(string.Empty, OnKeysChanged));

    public static readonly DependencyProperty ActionProperty =
        DependencyProperty.Register(nameof(Action), typeof(string),
            typeof(ShortcutRow),
            new PropertyMetadata(string.Empty, OnActionChanged));

    public string Keys
    {
        get => (string)GetValue(KeysProperty);
        set => SetValue(KeysProperty, value);
    }

    public string Action
    {
        get => (string)GetValue(ActionProperty);
        set => SetValue(ActionProperty, value);
    }

    // ── Constructor ───────────────────────────────────────────────────────────

    public ShortcutRow()
    {
        InitializeComponent();
    }

    // ── Property change callbacks ─────────────────────────────────────────────

    private static void OnKeysChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        var self = (ShortcutRow)d;
        self.KeysText.Text = (string)e.NewValue;
    }

    private static void OnActionChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        var self = (ShortcutRow)d;
        self.ActionText.Text = (string)e.NewValue;
    }
}
