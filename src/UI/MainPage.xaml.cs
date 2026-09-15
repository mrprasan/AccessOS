// AccessOS/src/UI/MainPage.xaml.cs
//
// Code-behind for the AccessOS screen reader control panel.
//
// ACCESSOS-017: All CoreBridge calls are now live.
//   - Start  → CoreBridge.Initialize() + AccessOS C++ core runs
//   - Stop   → CoreBridge.Shutdown()
//   - Sliders/Voice → forwarded to C++ SpeechManager via bridge
//   - 500ms DispatcherTimer polls focused name/role and context type
//     and updates the UI panel in real time
//   - Verbosity ComboBox calls CoreBridge.SetVerbosity()
//   - Read Focused button calls CoreBridge.ReadFocused()

using System;
using System.Collections.Specialized;
using Microsoft.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using Windows.UI;

namespace AccessOS_UI;

public sealed partial class MainPage : Page
{
    private readonly ShellViewModel _vm  = new();
    private          CoreBridge?    _core;
    private          DispatcherTimer? _pollTimer;

    // ── Construction ─────────────────────────────────────────────────────────

    public MainPage()
    {
        InitializeComponent();
        BindViewModel();
        PopulateVoices();
        PopulateVerbosityCombo();
        PopulateBrowseReadUnitCombo();
        _vm.ActivityLog.CollectionChanged += ActivityLog_CollectionChanged;
    }

    // ── ViewModel binding ─────────────────────────────────────────────────────

    private void BindViewModel()
    {
        RefreshStateBar();
        FocusedNameText.Text    = _vm.FocusedName;
        FocusedRoleText.Text    = _vm.FocusedRole;
        FocusedValueText.Text   = _vm.FocusedValue;
        FocusedStateText.Text   = _vm.FocusedState;
        ContextTypeText.Text    = _vm.ContextType;
        RateSlider.Value        = _vm.SpeechRate;
        VolumeSlider.Value      = _vm.SpeechVolume * 100.0;

        _vm.PropertyChanged += (_, e) =>
        {
            switch (e.PropertyName)
            {
                case nameof(ShellViewModel.State):
                    RefreshStateBar();
                    break;
                case nameof(ShellViewModel.FocusedName):
                    FocusedNameText.Text  = _vm.FocusedName;  break;
                case nameof(ShellViewModel.FocusedRole):
                    FocusedRoleText.Text  = _vm.FocusedRole;  break;
                case nameof(ShellViewModel.FocusedValue):
                    FocusedValueText.Text = _vm.FocusedValue; break;
                case nameof(ShellViewModel.FocusedState):
                    FocusedStateText.Text = _vm.FocusedState; break;
                case nameof(ShellViewModel.ContextType):
                    ContextTypeText.Text  = _vm.ContextType;  break;
            }
        };
    }

    private void RefreshStateBar()
    {
        StateLabel.Text = _vm.StateLabel;

        var hex = _vm.StateColor.TrimStart('#');
        var r   = Convert.ToByte(hex[0..2], 16);
        var g   = Convert.ToByte(hex[2..4], 16);
        var b   = Convert.ToByte(hex[4..6], 16);
        StateDotBrush.Color = Color.FromArgb(0xFF, r, g, b);

        StartButton.IsEnabled  = _vm.CanStart;
        PauseButton.IsEnabled  = _vm.CanStop;
        StopButton.IsEnabled   = _vm.CanStop;
    }

    // ── Voice population ──────────────────────────────────────────────────────

    private void PopulateVoices()
    {
        try
        {
            var synth  = new Windows.Media.SpeechSynthesis.SpeechSynthesizer();
            var voices = Windows.Media.SpeechSynthesis.SpeechSynthesizer.AllVoices;

            foreach (var voice in voices)
            {
                _vm.Voices.Add(voice.DisplayName);
                VoiceComboBox.Items.Add(voice.DisplayName);
            }

            if (VoiceComboBox.Items.Count > 0)
            {
                var defaultName = synth.Voice?.DisplayName;
                VoiceComboBox.SelectedItem = defaultName ?? VoiceComboBox.Items[0];
                _vm.SelectedVoice = (string?)VoiceComboBox.SelectedItem;
            }

            synth.Dispose();
        }
        catch
        {
            VoiceComboBox.Items.Add("(System default)");
            VoiceComboBox.SelectedIndex = 0;
        }
    }

    // ── Verbosity ComboBox ────────────────────────────────────────────────────

    private void PopulateVerbosityCombo()
    {
        VerbosityComboBox.Items.Add("Minimal");
        VerbosityComboBox.Items.Add("Standard");
        VerbosityComboBox.Items.Add("Detailed");
        VerbosityComboBox.Items.Add("Developer");
        VerbosityComboBox.SelectedIndex = _vm.VerbosityLevel;
    }

    // ── Button handlers ───────────────────────────────────────────────────────

    private void StartButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            _core = new CoreBridge();
            _core.Initialize();
            _vm.State = ReaderState.Running;
            _vm.AppendLog($"[{Timestamp()}] Screen reader started.");
            StartPollTimer();
        }
        catch (Exception ex)
        {
            _vm.AppendLog($"[{Timestamp()}] ERROR: {ex.Message}");
            _core?.Dispose();
            _core = null;
        }
    }

    private void PauseButton_Click(object sender, RoutedEventArgs e)
    {
        if (_vm.State == ReaderState.Paused)
        {
            _vm.State = ReaderState.Running;
            _pollTimer?.Start();
        }
        else
        {
            _vm.State = ReaderState.Paused;
            _pollTimer?.Stop();
            _core?.SpeechStop();
        }

        PauseButton.Content = _vm.State == ReaderState.Paused ? "Resume" : "Pause";
        _vm.AppendLog($"[{Timestamp()}] Screen reader {_vm.StateLabel.ToLower()}.");
    }

    private void StopButton_Click(object sender, RoutedEventArgs e)
    {
        StopPollTimer();
        _core?.SpeechStop();
        _core?.Shutdown();
        _core?.Dispose();
        _core = null;

        _vm.State           = ReaderState.Stopped;
        _vm.FocusedName     = "(none)";
        _vm.FocusedRole     = "";
        _vm.FocusedValue    = "";
        _vm.FocusedState    = "";
        _vm.ContextType     = "Unknown";
        PauseButton.Content = "Pause";
        _vm.AppendLog($"[{Timestamp()}] Screen reader stopped.");
    }

    // ── Speech control handlers ───────────────────────────────────────────────

    private void VoiceComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        _vm.SelectedVoice = VoiceComboBox.SelectedItem as string;
        if (_core != null && _vm.SelectedVoice is string v)
            _core.SetVoice(v);
    }

    private void RateSlider_ValueChanged(object sender,
        Microsoft.UI.Xaml.Controls.Primitives.RangeBaseValueChangedEventArgs e)
    {
        _vm.SpeechRate     = e.NewValue;
        RateValueText.Text = $"{e.NewValue:F1}×";
        _core?.SetSpeechRate(e.NewValue);
    }

    private void VolumeSlider_ValueChanged(object sender,
        Microsoft.UI.Xaml.Controls.Primitives.RangeBaseValueChangedEventArgs e)
    {
        _vm.SpeechVolume    = e.NewValue / 100.0;
        VolumeValueText.Text = $"{(int)e.NewValue}%";
        _core?.SetSpeechVolume((int)e.NewValue);
    }

    private void VerbosityComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        int level = VerbosityComboBox.SelectedIndex;
        _vm.VerbosityLevel = level;
        _core?.SetVerbosity(level);
        _vm.AppendLog($"[{Timestamp()}] Verbosity: {_vm.VerbosityLabel}");
    }

    // ── Read Focused button ───────────────────────────────────────────────────

    private void ReadFocused_Click(object sender, RoutedEventArgs e)
    {
        if (_core == null) return;
        _core.ReadFocused();
        _vm.AppendLog($"[{Timestamp()}] Re-read focused element.");
    }

    // ── Poll timer (500 ms) ───────────────────────────────────────────────────

    private void StartPollTimer()
    {
        _pollTimer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(500) };
        _pollTimer.Tick += PollTimer_Tick;
        _pollTimer.Start();
    }

    private void StopPollTimer()
    {
        if (_pollTimer == null) return;
        _pollTimer.Stop();
        _pollTimer.Tick -= PollTimer_Tick;
        _pollTimer = null;
    }

    private void PollTimer_Tick(object? sender, object e)
    {
        if (_core == null || !_core.IsRunning) return;

        var name = _core.GetFocusedName();
        var role = _core.GetFocusedRole();
        var ctx  = _core.GetContextType();

        if (name != null && name != _vm.FocusedName)
        {
            _vm.FocusedName = name;
            _vm.AppendLog($"[{Timestamp()}] {role ?? ""}: {name}");
        }
        if (role != null) _vm.FocusedRole = role;
        if (ctx  != null) _vm.ContextType = ctx;
    }

    // ── Log handlers ─────────────────────────────────────────────────────────

    private void ClearLog_Click(object sender, RoutedEventArgs e)
    {
        _vm.ActivityLog.Clear();
        LogList.ItemsSource = null;
    }

    private void ActivityLog_CollectionChanged(object? sender,
        NotifyCollectionChangedEventArgs e)
    {
        LogList.ItemsSource = null;
        LogList.ItemsSource = _vm.ActivityLog;
        LogList.UpdateLayout();
        LogScroller.ChangeView(null, LogScroller.ScrollableHeight, null,
            disableAnimation: true);
    }

    // ── Browse Mode handlers ──────────────────────────────────────────────────

    private void BrowseModeToggle_Toggled(object sender, RoutedEventArgs e)
    {
        bool on = BrowseModeToggle.IsOn;
        if (_core != null) _core.BrowseToggle();
        _vm.AppendLog($"[{Timestamp()}] Browse mode {(on ? "enabled" : "disabled")}.");
    }

    private void BrowseAutoDetectToggle_Toggled(object sender, RoutedEventArgs e)
    {
        // Persisted as a setting; passed to C++ core when available
        _core?.SaveSetting("browse.auto_detect", BrowseAutoDetectToggle.IsOn ? "1" : "0");
    }

    private void BrowseReadUnitCombo_SelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        // reading unit: 0=character, 1=word, 2=sentence, 3=paragraph
        int idx = BrowseReadUnitCombo.SelectedIndex;
        _core?.SaveSetting("browse.read_unit", idx.ToString());
    }

    private void PopulateBrowseReadUnitCombo()
    {
        BrowseReadUnitCombo.Items.Add("Character");
        BrowseReadUnitCombo.Items.Add("Word");
        BrowseReadUnitCombo.Items.Add("Sentence");
        BrowseReadUnitCombo.Items.Add("Paragraph");
        BrowseReadUnitCombo.SelectedIndex = 1; // default: word
    }

    // ── Audio / Earcon handlers ───────────────────────────────────────────────

    private void EarconMasterToggle_Toggled(object sender, RoutedEventArgs e)
    {
        bool on = EarconMasterToggle.IsOn;
        _core?.SaveSetting("earcon.master_enable", on ? "1" : "0");
        _vm.AppendLog($"[{Timestamp()}] Earcons {(on ? "enabled" : "disabled")}.");
    }

    private void EarconVolumeSlider_ValueChanged(object sender,
        Microsoft.UI.Xaml.Controls.Primitives.RangeBaseValueChangedEventArgs e)
    {
        EarconVolumeValueText.Text = $"{(int)e.NewValue}%";
        _core?.SaveSetting("earcon.volume", ((int)e.NewValue).ToString());
    }

    private void EarconFocusToggle_Toggled(object sender, RoutedEventArgs e)
        => _core?.SaveSetting("earcon.focus", EarconFocusToggle.IsOn ? "1" : "0");

    private void EarconErrorToggle_Toggled(object sender, RoutedEventArgs e)
        => _core?.SaveSetting("earcon.error", EarconErrorToggle.IsOn ? "1" : "0");

    private void EarconLinkToggle_Toggled(object sender, RoutedEventArgs e)
        => _core?.SaveSetting("earcon.link", EarconLinkToggle.IsOn ? "1" : "0");

    private void EarconButtonToggle_Toggled(object sender, RoutedEventArgs e)
        => _core?.SaveSetting("earcon.button", EarconButtonToggle.IsOn ? "1" : "0");

    // ── Auto-start handlers ───────────────────────────────────────────────────

    private void AutoStartToggle_Loaded(object sender, RoutedEventArgs e)
    {
        // Sync toggle to current registry state without firing Toggled
        AutoStartToggle.Toggled -= AutoStartToggle_Toggled;
        AutoStartToggle.IsOn     = AutoStart.IsEnabled();
        AutoStartToggle.Toggled += AutoStartToggle_Toggled;
    }

    private void AutoStartToggle_Toggled(object sender, RoutedEventArgs e)
    {
        bool ok = AutoStartToggle.IsOn ? AutoStart.Enable() : AutoStart.Disable();
        if (!ok)
            _vm.AppendLog($"[{Timestamp()}] WARNING: Could not update auto-start registry entry.");
        else
            _vm.AppendLog($"[{Timestamp()}] Auto-start {(AutoStartToggle.IsOn ? "enabled" : "disabled")}.");
    }

    // ── Helpers ───────────────────────────────────────────────────────────────

    private static string Timestamp()
        => DateTime.Now.ToString("HH:mm:ss");
}
