// AccessOS/src/Core/Reader/AccessReader.cpp

#include "AccessReader.h"

namespace AccessOS {

// ── Constructor ───────────────────────────────────────────────────────────────

AccessReader::AccessReader(ContextEngine*        contextEngine,
                           FocusManager*         focusManager,
                           SpeechManager*        speechManager,
                           Audio::EarconManager* earconManager)
    : m_contextEngine(contextEngine)
    , m_focusManager(focusManager)
    , m_speechManager(speechManager)
    , m_earconManager(earconManager)
{
}

// ── Lifecycle ─────────────────────────────────────────────────────────────────

void AccessReader::Initialize() noexcept {
    m_running.store(true);
}

void AccessReader::Shutdown() noexcept {
    m_running.store(false);
}

// ── IEventListener ────────────────────────────────────────────────────────────

void AccessReader::OnEvent(const AccessEvent& event) {
    if (!m_running.load()) return;

    switch (event.type) {
    case AccessEventType::FocusChanged:
        HandleFocusChanged(event);
        break;

    case AccessEventType::TextChanged:
        HandleTextChanged(event);
        break;

    case AccessEventType::ValueChanged:
    case AccessEventType::StateChanged:
    case AccessEventType::NameChanged:
        HandlePropertyChanged(event);
        break;

    case AccessEventType::LiveRegionChanged:
    case AccessEventType::NotificationRaised:
        HandleLiveRegion(event);
        break;

    // Alert / AlertDialog nodes arrive as FocusChanged or StructureChanged.
    // When the element role is Alert/AlertDialog, promote to alert handling.
    case AccessEventType::WindowActivated:
    case AccessEventType::WindowOpened:
        if (event.element.role == AccessRole::AlertDialog ||
            event.element.role == AccessRole::Alert) {
            HandleLiveRegion(event);
        }
        break;

    default:
        break;
    }
}

// ── Event handlers ────────────────────────────────────────────────────────────

void AccessReader::HandleFocusChanged(const AccessEvent& event) {
    const AccessNode& node = event.element;

    // Update context engine — fires observers if context changed.
    if (m_contextEngine) {
        m_contextEngine->Update(node);
    }

    // Retrieve current context and previous window title.
    AppContext ctx;
    if (m_contextEngine) {
        ctx = m_contextEngine->GetCurrent();
    }

    std::string prevTitle;
    {
        std::unique_lock<std::mutex> lk(m_titleMutex);
        prevTitle = m_lastWindowTitle;
        m_lastWindowTitle = node.windowTitle;
    }

    // Fire earcon for the focused element's role
    using AR = AccessRole;
    using EI = Audio::EarconId;
    switch (node.role) {
    case AR::Button:
    case AR::SplitButton:
    case AR::ToggleButton:  PlayEarcon(EI::Button);    break;
    case AR::Link:          PlayEarcon(EI::Link);      break;
    case AR::CheckBox:      PlayEarcon(EI::Checkbox);  break;
    case AR::ComboBox:      PlayEarcon(EI::Combobox);  break;
    case AR::Edit:
    case AR::MultiLineEdit: PlayEarcon(EI::TextInput); break;
    case AR::MenuItem:      PlayEarcon(EI::MenuItem);  break;
    case AR::ListItem:      PlayEarcon(EI::ListItem);  break;
    default:                PlayEarcon(EI::FocusEnter); break;
    }

    // Build and speak the focus announcement.
    SpeechPolicy pol = GetPolicy();
    Announcement ann = AnnouncementEngine::BuildFocusAnnouncement(
        node, ctx, pol, prevTitle);

    Speak(ann);
}

void AccessReader::HandlePropertyChanged(const AccessEvent& event) {
    AppContext ctx;
    if (m_contextEngine) {
        ctx = m_contextEngine->GetCurrent();
    }

    // Fire earcon for state changes
    if (event.type == AccessEventType::StateChanged) {
        using S  = AccessState;
        using EI = Audio::EarconId;
        const AccessState& st = event.element.state;
        if ((st & S::Checked) != S::None)    PlayEarcon(EI::Checked);
        if ((st & S::Collapsed) != S::None)  PlayEarcon(EI::Collapsed);
        if ((st & S::Expanded) != S::None)   PlayEarcon(EI::Expanded);
    }

    SpeechPolicy pol = GetPolicy();
    Announcement ann = AnnouncementEngine::BuildPropertyAnnouncement(
        event.element, ctx, pol);

    Speak(ann);
}

void AccessReader::HandleLiveRegion(const AccessEvent& event) {
    AppContext ctx;
    if (m_contextEngine) {
        ctx = m_contextEngine->GetCurrent();
    }

    // Alert earcon
    using AR = AccessRole;
    using EI = Audio::EarconId;
    if (event.element.role == AR::Alert || event.element.role == AR::AlertDialog) {
        PlayEarcon(EI::Alert);
    }

    Announcement ann = AnnouncementEngine::BuildAlertAnnouncement(
        event.element, ctx);

    // If there's extra info (notification text), append it.
    if (!event.extraInfo.empty()) {
        ann.AddText(event.extraInfo);
    }

    Speak(ann);
}

// ── Speak ─────────────────────────────────────────────────────────────────────

void AccessReader::Speak(const Announcement& ann) {
    if (!m_speechManager) return;
    if (ann.IsEmpty()) return;

    const std::string text = ann.Flatten();
    if (text.empty()) return;

    m_speechManager->SpeakText(text, ann.priority, ann.cancelPrevious);
}

// ── Control API ───────────────────────────────────────────────────────────────

void AccessReader::SetPolicy(SpeechPolicy policy) {
    std::unique_lock<std::mutex> lk(m_policyMutex);
    m_policy = policy;
}

SpeechPolicy AccessReader::GetPolicy() const {
    std::unique_lock<std::mutex> lk(m_policyMutex);
    return m_policy;
}

void AccessReader::StopSpeech() {
    if (m_speechManager) {
        m_speechManager->Stop();
    }
}

void AccessReader::ReadFocused() {
    if (!m_focusManager || !m_running.load()) return;

    auto focused = m_focusManager->GetCurrentFocus();
    if (!focused.has_value()) return;

    AppContext ctx;
    if (m_contextEngine) {
        ctx = m_contextEngine->GetCurrent();
    }

    SpeechPolicy pol = GetPolicy();
    Announcement ann = AnnouncementEngine::BuildFocusAnnouncement(
        focused.value(), ctx, pol);

    Speak(ann);
}

std::string AccessReader::LastWindowTitle() const {
    std::unique_lock<std::mutex> lk(m_titleMutex);
    return m_lastWindowTitle;
}

// ── Earcon helper ─────────────────────────────────────────────────────────────

void AccessReader::PlayEarcon(Audio::EarconId id) {
    if (m_earconManager) m_earconManager->Play(id);
}

// ── Say All ───────────────────────────────────────────────────────────────────

void AccessReader::StartSayAll() {
    if (!m_cursor.HasDocument()) return;

    // Stop any existing say-all
    StopSayAll();

    m_sayAll.SetCursor(&m_cursor);
    m_sayAll.SetSpeechManager(m_speechManager);

    std::lock_guard<std::mutex> lk(m_sayAllMutex);
    m_sayAllThread = std::thread([this]() {
        m_sayAll.Run();
    });
    m_sayAllThread.detach();
}

void AccessReader::StopSayAll() {
    m_sayAll.Stop();
    // Give thread a moment to stop; it is detached so we can't join
    std::lock_guard<std::mutex> lk(m_sayAllMutex);
    if (m_speechManager) m_speechManager->Stop();
}

bool AccessReader::IsSayAllActive() const noexcept {
    return m_sayAll.IsRunning();
}

// ── SpeakRaw helper ───────────────────────────────────────────────────────────

void AccessReader::SpeakRaw(const std::string& text) {
    if (!m_speechManager || text.empty()) return;
    m_speechManager->SpeakText(text, SpeechPriority::Normal, true);
}

// ── Browse Mode ───────────────────────────────────────────────────────────────

bool AccessReader::ToggleBrowseMode() {
    bool newVal = !m_browseMode.load();
    m_browseMode.store(newVal);
    if (newVal) {
        // Build a simple virtual document from focused element text
        m_browseDoc = std::make_shared<Browse::VirtualDocument>();
        // Seed with focused element if available
        if (m_focusManager) {
            auto focused = m_focusManager->GetCurrentFocus();
            if (focused.has_value()) {
                Browse::VirtualNode node;
                node.text = focused->name;
                node.role = Browse::VirtualRole::Text;
                m_browseDoc->AddNode(node);
            }
        }
        m_cursor.SetDocument(m_browseDoc);
        SpeakRaw("Browse mode on");
    } else {
        SpeakRaw("Browse mode off");
    }
    return newVal;
}

void AccessReader::BrowseMoveNext() {
    if (!m_cursor.HasDocument()) return;
    if (m_cursor.MoveNextElement()) {
        SpeakRaw(m_cursor.AnnouncePosition());
    } else {
        PlayEarcon(Audio::EarconId::Boundary);
        SpeakRaw("Bottom of page");
    }
}

void AccessReader::BrowseMovePrev() {
    if (!m_cursor.HasDocument()) return;
    if (m_cursor.MovePrevElement()) {
        SpeakRaw(m_cursor.AnnouncePosition());
    } else {
        PlayEarcon(Audio::EarconId::Boundary);
        SpeakRaw("Top of page");
    }
}

void AccessReader::BrowseMoveNextHeading() {
    if (!m_cursor.HasDocument()) return;
    if (m_cursor.MoveNextHeading()) {
        SpeakRaw(m_cursor.ReadCurrentNode());
    } else {
        PlayEarcon(Audio::EarconId::Boundary);
        SpeakRaw("No next heading");
    }
}

void AccessReader::BrowseMovePrevHeading() {
    if (!m_cursor.HasDocument()) return;
    if (m_cursor.MovePrevHeading()) {
        SpeakRaw(m_cursor.ReadCurrentNode());
    } else {
        PlayEarcon(Audio::EarconId::Boundary);
        SpeakRaw("No previous heading");
    }
}

// ── Table reading ─────────────────────────────────────────────────────────────

bool AccessReader::HasActiveTable() const {
    return m_tableNav.HasTable();
}

std::string AccessReader::GetCurrentTableCell() const {
    if (!m_tableNav.HasTable()) return "";
    return m_tableNav.AnnounceCurrentCell(Table::HeaderMode::Both);
}

std::string AccessReader::TableMoveNext() {
    if (!m_tableNav.HasTable()) return "";
    auto r = m_tableNav.MoveNextCell();
    if (r == Table::MoveResult::Boundary) {
        PlayEarcon(Audio::EarconId::Boundary);
        return "Table end";
    }
    return m_tableNav.AnnounceCurrentCell(Table::HeaderMode::Both);
}

std::string AccessReader::TableMovePrev() {
    if (!m_tableNav.HasTable()) return "";
    auto r = m_tableNav.MovePrevCell();
    if (r == Table::MoveResult::Boundary) {
        PlayEarcon(Audio::EarconId::Boundary);
        return "Table start";
    }
    return m_tableNav.AnnounceCurrentCell(Table::HeaderMode::Both);
}

std::string AccessReader::TableMoveNextRow() {
    if (!m_tableNav.HasTable()) return "";
    auto r = m_tableNav.MoveNextRow();
    if (r == Table::MoveResult::Boundary) {
        PlayEarcon(Audio::EarconId::Boundary);
        return "Last row";
    }
    return m_tableNav.AnnounceCurrentCell(Table::HeaderMode::Both);
}

std::string AccessReader::TableMovePrevRow() {
    if (!m_tableNav.HasTable()) return "";
    auto r = m_tableNav.MovePrevRow();
    if (r == Table::MoveResult::Boundary) {
        PlayEarcon(Audio::EarconId::Boundary);
        return "First row";
    }
    return m_tableNav.AnnounceCurrentCell(Table::HeaderMode::Both);
}

// ── Typing Echo (ACCESSOS-032) ────────────────────────────────────────────────

void AccessReader::SetTypingEchoMode(TypingEchoMode mode) {
    m_typingEcho.store(static_cast<uint8_t>(mode));
}

AccessReader::TypingEchoMode AccessReader::GetTypingEchoMode() const {
    return static_cast<TypingEchoMode>(m_typingEcho.load());
}

void AccessReader::HandleTextChanged(const AccessEvent& event) {
    auto mode = static_cast<TypingEchoMode>(m_typingEcho.load());
    if (mode == TypingEchoMode::Off) return;

    // Ignore protected (password) fields
    if (HasState(event.element.state, AccessState::Protected)) return;

    const std::string& newText = event.element.value;
    std::string prev;
    {
        std::lock_guard<std::mutex> lk(m_typingMutex);
        prev = m_lastTypedText;
        m_lastTypedText = newText;
    }

    // Determine what was typed since last event
    if (newText.size() <= prev.size()) return; // deletion — don't echo

    std::string typed = newText.substr(prev.size());
    if (typed.empty()) return;

    bool echoChar = (mode == TypingEchoMode::Char || mode == TypingEchoMode::Both);
    bool echoWord = (mode == TypingEchoMode::Word || mode == TypingEchoMode::Both);

    if (echoChar) {
        // Speak the last typed character
        SpeakRaw(typed.substr(typed.size() - 1));
    }

    if (echoWord) {
        // If the typed text ends with a word delimiter, speak the completed word
        char last = typed.back();
        if (last == ' ' || last == '\n' || last == '\t' ||
            last == '.' || last == ',' || last == '?' || last == '!') {
            // Extract last word from newText before the delimiter
            std::string text = newText.substr(0, newText.size() - 1);
            size_t pos = text.find_last_of(" \t\n.,?!");
            std::string word = (pos == std::string::npos) ? text : text.substr(pos + 1);
            if (!word.empty()) SpeakRaw(word);
        }
    }
}

// ── Clipboard Reading (ACCESSOS-033) ─────────────────────────────────────────

bool AccessReader::ReadClipboard() {
    std::string text = ClipboardReader::Read();
    if (text.empty()) {
        SpeakRaw("Clipboard is empty");
        return false;
    }
    SpeakRaw(text);
    return true;
}

} // namespace AccessOS
