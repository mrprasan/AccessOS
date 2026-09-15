// AccessOS/src/UI/ShellViewModel.cs
//
// ShellViewModel — bindable state for the AccessOS shell UI.
//
// Why: Keeps all UI state in one place, separate from XAML code-behind.
//      Uses plain INotifyPropertyChanged — no external MVVM framework.
//
// Threading: All property setters must be called on the UI thread.
//            The core C++ engine raises events on a worker thread;
//            callers must DispatcherQueue.TryEnqueue before setting here.

using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace AccessOS_UI;

/// <summary>
/// Running/stopped state of the screen reader engine.
/// </summary>
public enum ReaderState
{
    Stopped,
    Running,
    Paused,
}

/// <summary>
/// Bindable ViewModel for the AccessOS shell.
/// </summary>
public sealed class ShellViewModel : INotifyPropertyChanged
{
    // ── INotifyPropertyChanged ────────────────────────────────────────────────

    public event PropertyChangedEventHandler? PropertyChanged;

    private void Set<T>(ref T field, T value, [CallerMemberName] string? name = null)
    {
        if (Equals(field, value)) return;
        field = value;
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
    }

    // ── Reader state ──────────────────────────────────────────────────────────

    private ReaderState _state = ReaderState.Stopped;
    public ReaderState State
    {
        get => _state;
        set
        {
            Set(ref _state, value);
            OnPropertyChanged(nameof(StateLabel));
            OnPropertyChanged(nameof(StateColor));
            OnPropertyChanged(nameof(CanStart));
            OnPropertyChanged(nameof(CanStop));
        }
    }

    public string StateLabel => State switch
    {
        ReaderState.Running => "Running",
        ReaderState.Paused  => "Paused",
        _                   => "Stopped",
    };

    public string StateColor => State switch
    {
        ReaderState.Running => "#22C55E",   // green-500
        ReaderState.Paused  => "#F59E0B",   // amber-500
        _                   => "#6B7280",   // gray-500
    };

    public bool CanStart => State == ReaderState.Stopped || State == ReaderState.Paused;
    public bool CanStop  => State == ReaderState.Running  || State == ReaderState.Paused;

    // ── Focused element readout ───────────────────────────────────────────────

    private string _focusedName = "(none)";
    public string FocusedName
    {
        get => _focusedName;
        set => Set(ref _focusedName, value);
    }

    private string _focusedRole = "";
    public string FocusedRole
    {
        get => _focusedRole;
        set => Set(ref _focusedRole, value);
    }

    private string _focusedValue = "";
    public string FocusedValue
    {
        get => _focusedValue;
        set => Set(ref _focusedValue, value);
    }

    private string _focusedState = "";
    public string FocusedState
    {
        get => _focusedState;
        set => Set(ref _focusedState, value);
    }

    // ── Context display ───────────────────────────────────────────────────────

    private string _contextType = "Unknown";
    /// <summary>Current application context type, e.g. "Browser", "Terminal".</summary>
    public string ContextType
    {
        get => _contextType;
        set => Set(ref _contextType, value);
    }

    // ── Verbosity ─────────────────────────────────────────────────────────────

    private int _verbosityLevel = 1;   // 0=Minimal 1=Standard 2=Detailed 3=Developer
    /// <summary>Active verbosity level (0–3).</summary>
    public int VerbosityLevel
    {
        get => _verbosityLevel;
        set => Set(ref _verbosityLevel, value);
    }

    public string VerbosityLabel => _verbosityLevel switch
    {
        0 => "Minimal",
        2 => "Detailed",
        3 => "Developer",
        _ => "Standard",
    };

    // ── Speech settings ───────────────────────────────────────────────────────

    private ObservableCollection<string> _voices = new();
    public ObservableCollection<string> Voices => _voices;

    private string? _selectedVoice;
    public string? SelectedVoice
    {
        get => _selectedVoice;
        set => Set(ref _selectedVoice, value);
    }

    private double _speechRate = 1.0;       // 0.5 – 3.0
    public double SpeechRate
    {
        get => _speechRate;
        set => Set(ref _speechRate, value);
    }

    private double _speechVolume = 1.0;     // 0.0 – 1.0
    public double SpeechVolume
    {
        get => _speechVolume;
        set => Set(ref _speechVolume, value);
    }

    // ── Log / activity feed ───────────────────────────────────────────────────

    private ObservableCollection<string> _activityLog = new();
    public ObservableCollection<string> ActivityLog => _activityLog;

    public void AppendLog(string entry)
    {
        if (_activityLog.Count >= 200)
            _activityLog.RemoveAt(0);
        _activityLog.Add(entry);
    }

    // ── Helpers ───────────────────────────────────────────────────────────────

    private void OnPropertyChanged([CallerMemberName] string? name = null)
        => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
}
