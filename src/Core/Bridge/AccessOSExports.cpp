// AccessOS/src/Core/Bridge/AccessOSExports.cpp
//
// Deliberately does NOT include AccessOSExports.h here.
// The extern "C" block in that header would swallow the C++ helper
// functions defined in this file. Instead we redeclare the minimal
// types needed and wrap only the exported functions in extern "C".

#include "../Speech/SpeechManager.h"
#include "../Speech/SapiSpeechEngine.h"
#include "../Speech/SpeechPriority.h"
#include "../Speech/SpeechPolicy.h"
#include "../Semantic/SemanticModel.h"
#include "../Events/EventEngine.h"
#include "../Context/ContextEngine.h"
#include "../Focus/FocusManager.h"
#include "../Reader/AccessReader.h"
#include "../Diagnostics/DiagnosticsManager.h"
#include "../Commands/CommandManager.h"
#include "../Commands/ShortcutManager.h"
#include "../Commands/KeyboardManager.h"
#include "../Commands/CommandRegistry.h"
#include "../Navigation/NavigationEngine.h"
#include "../Settings/SettingsManager.h"
#include "../Settings/SqliteSettingsStore.h"
#include "../Settings/SettingsApplier.h"
#include "../Logging/Logger.h"
#include "../Accessibility/UIAutomation/UIAIncludes.h"
#include "../Audio/EarconManager.h"
#include "../Audio/WaveEarconEngine.h"
#include "../Adapters/AdapterRegistry.h"
#include "../Adapters/UiaAdapter.h"
#include "../Adapters/BrowserAdapter.h"

#include <objbase.h>
#include <memory>
#include <string>
#include <mutex>
#include <cstring>

// ── Internal core instance ─────────────────────────────────────────────────────

static constexpr const char* kComp = "AccessOSExports";

struct CoreInstance {
    IUIAutomation*                               automation{ nullptr };
    std::unique_ptr<AccessOS::EventEngine>       eventEngine;
    std::unique_ptr<AccessOS::SemanticModel>     semanticModel;
    std::unique_ptr<AccessOS::ContextEngine>     contextEngine;
    std::unique_ptr<AccessOS::FocusManager>      focusManager;
    std::unique_ptr<AccessOS::SpeechManager>     speechManager;
    std::unique_ptr<AccessOS::AccessReader>       reader;
    std::unique_ptr<AccessOS::DiagnosticsManager> diagnostics;
    std::unique_ptr<AccessOS::CommandManager>     commandManager;
    std::unique_ptr<AccessOS::ShortcutManager>    shortcutManager;
    std::unique_ptr<AccessOS::NavigationEngine>   navigationEngine;
    std::unique_ptr<AccessOS::KeyboardManager>    keyboardManager;
    std::shared_ptr<AccessOS::SqliteSettingsStore>    settingsStore;
    std::unique_ptr<AccessOS::SettingsManager>        settingsMgr;
    std::unique_ptr<AccessOS::Audio::WaveEarconEngine> earconEngine;
    std::unique_ptr<AccessOS::Audio::EarconManager>   earconMgr;
    std::unique_ptr<AccessOS::Adapters::AdapterRegistry> adapterRegistry;
    bool                                              running{ false };
    mutable std::mutex                                mutex;

    ~CoreInstance() {
        if (keyboardManager) { keyboardManager->Uninstall(); }
        if (reader)          { reader->Shutdown(); }
        if (speechManager)   { speechManager->Shutdown(); }
        if (eventEngine)     { eventEngine->Shutdown(); }
        if (automation)      { automation->Release(); automation = nullptr; }
    }
};

// ── C++ helpers (NOT exported, NOT extern "C") ────────────────────────────────

static CoreInstance* ToCore(void* h) {
    return reinterpret_cast<CoreInstance*>(h);
}

static int CopyString(const std::string& src, char* buf, int bufSize) {
    if (!buf || bufSize <= 0) return -1;
    const size_t len = src.size();
    if (static_cast<size_t>(bufSize) <= len) return -2;
    std::memcpy(buf, src.c_str(), len + 1);
    return 0;
}

static const char* ContextTypeName(AccessOS::AppContextType t) noexcept {
    using T = AccessOS::AppContextType;
    switch (t) {
    case T::Browser:            return "Browser";
    case T::BrowserAddressBar:  return "BrowserAddressBar";
    case T::Document:           return "Document";
    case T::Spreadsheet:        return "Spreadsheet";
    case T::Presentation:       return "Presentation";
    case T::CodeEditor:         return "CodeEditor";
    case T::Terminal:           return "Terminal";
    case T::Dialog:             return "Dialog";
    case T::FileExplorer:       return "FileExplorer";
    case T::SystemTray:         return "SystemTray";
    case T::TaskBar:            return "TaskBar";
    case T::StartMenu:          return "StartMenu";
    case T::Email:              return "Email";
    case T::Chat:               return "Chat";
    case T::Calendar:           return "Calendar";
    case T::MediaPlayer:        return "MediaPlayer";
    case T::DesktopApp:         return "DesktopApp";
    default:                    return "Unknown";
    }
}

// ── Exported C functions ───────────────────────────────────────────────────────

extern "C" {

__declspec(dllexport) void* __cdecl AcosCreate(void) {
    auto* core = new (std::nothrow) CoreInstance();
    if (!core) return nullptr;

    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        ACOS_LOG_ERROR(kComp, "CoInitializeEx failed");
        delete core;
        return nullptr;
    }

    hr = CoCreateInstance(__uuidof(CUIAutomation8), nullptr,
                          CLSCTX_INPROC_SERVER,
                          __uuidof(IUIAutomation),
                          reinterpret_cast<void**>(&core->automation));
    if (FAILED(hr)) {
        ACOS_LOG_ERROR(kComp, "CoCreateInstance(UIAutomation) failed");
        delete core;
        return nullptr;
    }

    // ── Semantic model + event engine ─────────────────────────────────────────
    core->semanticModel = std::make_unique<AccessOS::SemanticModel>();
    core->eventEngine   = std::make_unique<AccessOS::EventEngine>();

    auto evResult = core->eventEngine->Initialize(core->automation);
    if (!evResult.IsOk()) {
        ACOS_LOG_ERROR(kComp, "EventEngine::Initialize failed");
        delete core;
        return nullptr;
    }
    core->eventEngine->AddListener(core->semanticModel.get());

    // ── Context engine + focus manager ────────────────────────────────────────
    core->contextEngine = std::make_unique<AccessOS::ContextEngine>();
    core->focusManager  = std::make_unique<AccessOS::FocusManager>();
    core->focusManager->Initialize(core->semanticModel.get());

    // ── Speech manager ────────────────────────────────────────────────────────
    core->speechManager = std::make_unique<AccessOS::SpeechManager>();
    auto spResult = core->speechManager->Initialize(
        std::make_unique<AccessOS::SapiSpeechEngine>());
    if (!spResult.IsOk()) {
        ACOS_LOG_ERROR(kComp, "SpeechManager::Initialize failed");
        delete core;
        return nullptr;
    }

    // ── Earcon engine ─────────────────────────────────────────────────────────
    core->earconEngine = std::make_unique<AccessOS::Audio::WaveEarconEngine>();
    core->earconMgr    = std::make_unique<AccessOS::Audio::EarconManager>(
                             std::shared_ptr<AccessOS::Audio::IAudioEngine>(
                                 core->earconEngine.get(),
                                 [](AccessOS::Audio::IAudioEngine*){})); // non-owning shared_ptr

    // ── AccessReader — wires everything together ──────────────────────────────
    core->reader = std::make_unique<AccessOS::AccessReader>(
        core->contextEngine.get(),
        core->focusManager.get(),
        core->speechManager.get(),
        core->earconMgr.get());
    core->reader->Initialize();

    // ── AdapterRegistry (ACCESSOS-030) ───────────────────────────────────────
    core->adapterRegistry = std::make_unique<AccessOS::Adapters::AdapterRegistry>();

    // UIA adapter wraps the existing EventEngine
    auto uiaAdapter = std::make_shared<AccessOS::Adapters::UiaAdapter>(
        std::shared_ptr<AccessOS::EventEngine>(
            core->eventEngine.get(),
            [](AccessOS::EventEngine*){})); // non-owning
    core->adapterRegistry->Register(uiaAdapter);

    // Browser adapter (NativeMessagingHost created lazily on Attach)
    core->adapterRegistry->Register(
        std::make_shared<AccessOS::Adapters::BrowserAdapter>());

    // Route adapter events → reader via pipeline callback
    core->adapterRegistry->SetPipelineCallback(
        [&core](const AccessOS::Adapters::AdapterEvent& ae) {
            // Convert AdapterEvent → AccessEvent and inject into reader
            AccessOS::AccessEvent evt;
            evt.element.name = ae.elementName;
            evt.timestampMs  = ae.timestampMs;
            if (ae.kind == AccessOS::Adapters::AdapterEventKind::BrowserFocus ||
                ae.kind == AccessOS::Adapters::AdapterEventKind::FocusChanged) {
                evt.type = AccessOS::AccessEventType::FocusChanged;
            } else if (ae.kind == AccessOS::Adapters::AdapterEventKind::Alert) {
                evt.type = AccessOS::AccessEventType::NotificationRaised;
            } else {
                evt.type = AccessOS::AccessEventType::Unknown;
            }
            if (core->reader) core->reader->OnEvent(evt);
        });

    // Register reader as an event listener so UIA events flow through directly.
    core->eventEngine->AddListener(core->reader.get());

    // ── Commands + navigation + keyboard ─────────────────────────────────────
    core->navigationEngine  = std::make_unique<AccessOS::NavigationEngine>(
                                  core->semanticModel.get());
    core->commandManager    = std::make_unique<AccessOS::CommandManager>();
    core->shortcutManager   = std::make_unique<AccessOS::ShortcutManager>();

    AccessOS::CommandRegistry::RegisterAll(
        *core->commandManager,
        core->reader.get(),
        core->navigationEngine.get(),
        core->speechManager.get(),
        core->focusManager.get());

    AccessOS::CommandRegistry::BindDefaults(*core->shortcutManager);

    core->keyboardManager = std::make_unique<AccessOS::KeyboardManager>(
        core->shortcutManager.get(),
        core->commandManager.get());
    core->keyboardManager->Install();

    // ── Settings — load from %APPDATA%\AccessOS\settings.db ──────────────────
    {
        char appData[MAX_PATH] = {};
        if (GetEnvironmentVariableA("APPDATA", appData, MAX_PATH) > 0) {
            std::string dbDir = std::string(appData) + "\\AccessOS";
            CreateDirectoryA(dbDir.c_str(), nullptr);
            std::string dbPath = dbDir + "\\settings.db";

            core->settingsStore = std::make_shared<AccessOS::SqliteSettingsStore>();
            if (core->settingsStore->Open(dbPath)) {
                core->settingsMgr = std::make_unique<AccessOS::SettingsManager>(
                    core->settingsStore);
                AccessOS::SettingsApplier::Apply(
                    *core->settingsMgr,
                    core->speechManager.get(),
                    core->reader.get(),
                    core->shortcutManager.get(),
                    core->commandManager.get());
            }
        }
    }

    // ── Diagnostics ───────────────────────────────────────────────────────────
    core->diagnostics = std::make_unique<AccessOS::DiagnosticsManager>();

    core->running = true;
    ACOS_LOG_INFO(kComp, "Core instance created");
    return core;
}

__declspec(dllexport) void __cdecl AcosDestroy(void* handle) {
    auto* core = ToCore(handle);
    if (!core) return;
    delete core;
    CoUninitialize();
    ACOS_LOG_INFO(kComp, "Core instance destroyed");
}

__declspec(dllexport) int __cdecl AcosIsRunning(void* handle) {
    auto* core = ToCore(handle);
    return (core && core->running) ? 1 : 0;
}

__declspec(dllexport) int __cdecl AcosSpeakText(void* handle,
                                                  const char* text,
                                                  int priority,
                                                  int cancelPrevious)
{
    auto* core = ToCore(handle);
    if (!core || !text) return -1;

    AccessOS::SpeechPriority prio;
    switch (priority) {
        case 0:  prio = AccessOS::SpeechPriority::Low;      break;
        case 2:  prio = AccessOS::SpeechPriority::High;     break;
        case 3:  prio = AccessOS::SpeechPriority::Critical; break;
        default: prio = AccessOS::SpeechPriority::Normal;   break;
    }
    core->speechManager->SpeakText(text, prio, cancelPrevious != 0);
    return 0;
}

__declspec(dllexport) int __cdecl AcosSpeechStop(void* handle) {
    auto* core = ToCore(handle);
    if (!core) return -1;
    core->speechManager->Stop();
    return 0;
}

__declspec(dllexport) int __cdecl AcosSpeechSetRate(void* handle, int rate) {
    auto* core = ToCore(handle);
    if (!core) return -1;
    core->speechManager->SetRate(rate);
    return 0;
}

__declspec(dllexport) int __cdecl AcosSpeechSetVolume(void* handle, int volume) {
    auto* core = ToCore(handle);
    if (!core) return -1;
    core->speechManager->SetVolume(volume);
    return 0;
}

__declspec(dllexport) int __cdecl AcosSpeechSetVoice(void* handle,
                                                       const char* voiceId)
{
    auto* core = ToCore(handle);
    if (!core || !voiceId) return -1;
    core->speechManager->SetVoice(voiceId);
    return 0;
}

__declspec(dllexport) int __cdecl AcosGetFocusedName(void* handle,
                                                       char* bufOut,
                                                       int   bufSize)
{
    auto* core = ToCore(handle);
    if (!core) return -1;
    auto focused = core->semanticModel->GetFocused();
    if (!focused.has_value()) return -2;
    return CopyString(focused->name, bufOut, bufSize);
}

__declspec(dllexport) int __cdecl AcosGetFocusedRole(void* handle,
                                                       char* bufOut,
                                                       int   bufSize)
{
    auto* core = ToCore(handle);
    if (!core) return -1;
    auto focused = core->semanticModel->GetFocused();
    if (!focused.has_value()) return -2;

    static const char* kRoleNames[] = {
        "Unknown","Window","Dialog","Pane","Group","Document","ScrollArea",
        "MenuBar","Menu","MenuItem","ToolBar","TabControl","Tab","Tree","TreeItem",
        "Button","SplitButton","ToggleButton","CheckBox","RadioButton","ComboBox",
        "ListBox","ListItem","Slider","Spinner","ProgressBar","ScrollBar","Separator",
        "Edit","MultiLineEdit","PasswordEdit","StaticText","Heading",
        "Link","Image","Figure","Table","TableRow","TableCell",
        "TableColumnHeader","TableRowHeader","List","ListItem","Form","FormField",
        "Landmark","Region","Banner","Navigation","Main","Complementary",
        "ContentInfo","Search","Alert","AlertDialog","Status","Log",
        "Marquee","Timer","Tooltip","Application","Desktop",
    };
    const int idx  = static_cast<int>(focused->role);
    const char* nm = (idx >= 0 && idx < static_cast<int>(std::size(kRoleNames)))
                     ? kRoleNames[idx] : "Unknown";
    return CopyString(nm, bufOut, bufSize);
}

__declspec(dllexport) int __cdecl AcosInjectBrowserFocus(void* handle,
                                                          const char* name,
                                                          const char* role,
                                                          const char* pageUrl)
{
    auto* core = ToCore(handle);
    if (!core || !name) return -1;

    AccessOS::AccessEvent evt;
    evt.type              = AccessOS::AccessEventType::FocusChanged;
    evt.element.name      = name;
    evt.element.value     = "";
    evt.element.processId = 0;
    evt.element.isValid   = true;
    evt.timestampMs       = 0;

    static const struct { const char* key; AccessOS::AccessRole val; } kMap[] = {
        {"button",      AccessOS::AccessRole::Button},
        {"checkbox",    AccessOS::AccessRole::CheckBox},
        {"combobox",    AccessOS::AccessRole::ComboBox},
        {"edit",        AccessOS::AccessRole::Edit},
        {"heading",     AccessOS::AccessRole::Heading},
        {"link",        AccessOS::AccessRole::Link},
        {"listitem",    AccessOS::AccessRole::ListItem},
        {"radiobutton", AccessOS::AccessRole::RadioButton},
        {"statictext",  AccessOS::AccessRole::StaticText},
    };
    evt.element.role = AccessOS::AccessRole::Unknown;
    if (role) {
        for (const auto& m : kMap) {
            if (std::strcmp(role, m.key) == 0) {
                evt.element.role = m.val;
                break;
            }
        }
    }
    if (pageUrl) evt.element.description = pageUrl;

    core->semanticModel->OnEvent(evt);
    // Also route through reader for speech.
    if (core->reader) core->reader->OnEvent(evt);
    return 0;
}

__declspec(dllexport) int __cdecl AcosInjectBrowserPageLoad(void* handle,
                                                             const char* url,
                                                             const char* title)
{
    auto* core = ToCore(handle);
    if (!core) return -1;

    AccessOS::AccessEvent evt;
    evt.type              = AccessOS::AccessEventType::StructureChanged;
    evt.element.id        = 1;
    evt.element.role      = AccessOS::AccessRole::Document;
    evt.element.name      = title ? title : "";
    evt.element.value     = url   ? url   : "";
    evt.element.processId = 0;
    evt.element.isValid   = true;
    evt.timestampMs       = 0;

    core->semanticModel->OnEvent(evt);
    return 0;
}

// ── New exports (ACCESSOS-016) ────────────────────────────────────────────────

__declspec(dllexport) int __cdecl AcosReadFocused(void* handle) {
    auto* core = ToCore(handle);
    if (!core || !core->reader) return -1;
    core->reader->ReadFocused();
    return 0;
}

__declspec(dllexport) int __cdecl AcosSetVerbosity(void* handle, int level) {
    auto* core = ToCore(handle);
    if (!core || !core->reader) return -1;

    AccessOS::SpeechPolicy pol = core->reader->GetPolicy();
    switch (level) {
    case 0:
        pol.verbosity           = AccessOS::VerbosityLevel::Minimal;
        pol.announceRole        = false;
        pol.announceState       = false;
        pol.announceValue       = false;
        pol.announceDescription = false;
        pol.announcePosition    = false;
        break;
    case 2:
        pol.verbosity           = AccessOS::VerbosityLevel::Detailed;
        pol.announceRole        = true;
        pol.announceState       = true;
        pol.announceValue       = true;
        pol.announceDescription = true;
        pol.announcePosition    = true;
        break;
    case 3:
        pol.verbosity           = AccessOS::VerbosityLevel::Developer;
        pol.announceRole        = true;
        pol.announceState       = true;
        pol.announceValue       = true;
        pol.announceDescription = true;
        pol.announcePosition    = true;
        pol.announceHelpText    = true;
        pol.announceShortcut    = true;
        break;
    default:  // 1 = Standard
        pol.verbosity           = AccessOS::VerbosityLevel::Standard;
        pol.announceRole        = true;
        pol.announceState       = true;
        pol.announceValue       = true;
        pol.announceDescription = false;
        pol.announcePosition    = false;
        break;
    }
    core->reader->SetPolicy(pol);
    core->speechManager->SetPolicy(pol);
    return 0;
}

__declspec(dllexport) int __cdecl AcosGetContextType(void* handle,
                                                       char* bufOut,
                                                       int   bufSize)
{
    auto* core = ToCore(handle);
    if (!core || !core->contextEngine) return -1;
    AccessOS::AppContext ctx = core->contextEngine->GetCurrent();
    return CopyString(ContextTypeName(ctx.type), bufOut, bufSize);
}

// ── Diagnostics exports (ACCESSOS-018) ───────────────────────────────────────

__declspec(dllexport) int __cdecl AcosGetDiagnostics(
    void*     handle,
    uint64_t* outEventsProcessed,
    uint64_t* outFocusChanges,
    uint64_t* outSpeechUtterances,
    uint64_t* outContextSwitches,
    uint64_t* outUptimeMs)
{
    auto* core = ToCore(handle);
    if (!core || !core->diagnostics) return -1;

    const AccessOS::DiagnosticsSnapshot snap = core->diagnostics->GetSnapshot();
    if (outEventsProcessed)  *outEventsProcessed  = snap.eventsProcessed;
    if (outFocusChanges)     *outFocusChanges      = snap.focusChanges;
    if (outSpeechUtterances) *outSpeechUtterances  = snap.speechUtterances;
    if (outContextSwitches)  *outContextSwitches   = snap.contextSwitches;
    if (outUptimeMs)         *outUptimeMs          = snap.uptimeMs;
    return 0;
}

__declspec(dllexport) int __cdecl AcosSetLogFile(void* handle, const char* path) {
    auto* core = ToCore(handle);
    if (!core || !core->diagnostics || !path) return -1;
    return core->diagnostics->SetLogFile(path) ? 0 : -2;
}

__declspec(dllexport) int __cdecl AcosResetCounters(void* handle) {
    auto* core = ToCore(handle);
    if (!core || !core->diagnostics) return -1;
    core->diagnostics->ResetCounters();
    return 0;
}

// ── Settings exports (ACCESSOS-020) ──────────────────────────────────────────

__declspec(dllexport) int __cdecl AcosSettingsGet(void*  handle,
                                                    const char* key,
                                                    char*  bufOut,
                                                    int    bufSize)
{
    auto* core = ToCore(handle);
    if (!core || !core->settingsMgr || !key) return -1;
    auto val = core->settingsMgr->Store().Get(key);
    if (!val.has_value()) return -2;
    return CopyString(*val, bufOut, bufSize);
}

__declspec(dllexport) int __cdecl AcosSettingsSet(void*  handle,
                                                    const char* key,
                                                    const char* value)
{
    auto* core = ToCore(handle);
    if (!core || !core->settingsMgr || !key || !value) return -1;
    core->settingsMgr->Store().Set(key, value);
    return 0;
}

__declspec(dllexport) int __cdecl AcosSettingsSave(void* handle) {
    auto* core = ToCore(handle);
    if (!core || !core->settingsMgr) return -1;
    AccessOS::SettingsApplier::Save(*core->settingsMgr, core->speechManager.get());
    return 0;
}

// ── Say All exports (ACCESSOS-031) ───────────────────────────────────────────

__declspec(dllexport) int __cdecl AcosSayAll(void* handle) {
    auto* core = ToCore(handle);
    if (!core || !core->reader) return -1;
    core->reader->StartSayAll();
    return 0;
}

__declspec(dllexport) int __cdecl AcosSayAllStop(void* handle) {
    auto* core = ToCore(handle);
    if (!core || !core->reader) return -1;
    core->reader->StopSayAll();
    return 0;
}

__declspec(dllexport) int __cdecl AcosSayAllIsRunning(void* handle) {
    auto* core = ToCore(handle);
    if (!core || !core->reader) return 0;
    return core->reader->IsSayAllActive() ? 1 : 0;
}

// ── Typing Echo exports (ACCESSOS-032) ───────────────────────────────────────

__declspec(dllexport) int __cdecl AcosSetTypingEcho(void* handle, int mode) {
    auto* core = ToCore(handle);
    if (!core || !core->reader) return -1;
    using T = AccessOS::AccessReader::TypingEchoMode;
    T m = (mode == 1) ? T::Char : (mode == 2) ? T::Word : (mode == 3) ? T::Both : T::Off;
    core->reader->SetTypingEchoMode(m);
    return 0;
}

__declspec(dllexport) int __cdecl AcosGetTypingEcho(void* handle) {
    auto* core = ToCore(handle);
    if (!core || !core->reader) return 0;
    return static_cast<int>(core->reader->GetTypingEchoMode());
}

// ── Clipboard Reading exports (ACCESSOS-033) ──────────────────────────────────

__declspec(dllexport) int __cdecl AcosReadClipboard(void* handle) {
    auto* core = ToCore(handle);
    if (!core || !core->reader) return -1;
    return core->reader->ReadClipboard() ? 0 : 1;
}

// ── Browse Mode exports (ACCESSOS-028) ────────────────────────────────────────

__declspec(dllexport) int __cdecl AcosBrowseToggle(void* handle) {
    auto* core = ToCore(handle);
    if (!core || !core->reader) return -1;
    return core->reader->ToggleBrowseMode() ? 1 : 0;
}

__declspec(dllexport) int __cdecl AcosBrowseIsActive(void* handle) {
    auto* core = ToCore(handle);
    if (!core || !core->reader) return 0;
    return core->reader->IsBrowseMode() ? 1 : 0;
}

__declspec(dllexport) int __cdecl AcosBrowseMoveNext(void* handle) {
    auto* core = ToCore(handle);
    if (!core || !core->reader) return -1;
    core->reader->BrowseMoveNext();
    return 0;
}

__declspec(dllexport) int __cdecl AcosBrowseMovePrev(void* handle) {
    auto* core = ToCore(handle);
    if (!core || !core->reader) return -1;
    core->reader->BrowseMovePrev();
    return 0;
}

__declspec(dllexport) int __cdecl AcosBrowseMoveNextHeading(void* handle) {
    auto* core = ToCore(handle);
    if (!core || !core->reader) return -1;
    core->reader->BrowseMoveNextHeading();
    return 0;
}

__declspec(dllexport) int __cdecl AcosBrowseMovePrevHeading(void* handle) {
    auto* core = ToCore(handle);
    if (!core || !core->reader) return -1;
    core->reader->BrowseMovePrevHeading();
    return 0;
}

// ── Table Reading exports (ACCESSOS-029) ──────────────────────────────────────

__declspec(dllexport) int __cdecl AcosTableGetCell(void* handle,
                                                     char* bufOut, int bufSize) {
    auto* core = ToCore(handle);
    if (!core || !core->reader) return -1;
    if (!core->reader->HasActiveTable()) return -1;
    return CopyString(core->reader->GetCurrentTableCell(), bufOut, bufSize);
}

__declspec(dllexport) int __cdecl AcosTableMoveNext(void* handle,
                                                      char* bufOut, int bufSize) {
    auto* core = ToCore(handle);
    if (!core || !core->reader) return -1;
    return CopyString(core->reader->TableMoveNext(), bufOut, bufSize);
}

__declspec(dllexport) int __cdecl AcosTableMovePrev(void* handle,
                                                      char* bufOut, int bufSize) {
    auto* core = ToCore(handle);
    if (!core || !core->reader) return -1;
    return CopyString(core->reader->TableMovePrev(), bufOut, bufSize);
}

__declspec(dllexport) int __cdecl AcosTableMoveNextRow(void* handle,
                                                         char* bufOut, int bufSize) {
    auto* core = ToCore(handle);
    if (!core || !core->reader) return -1;
    return CopyString(core->reader->TableMoveNextRow(), bufOut, bufSize);
}

__declspec(dllexport) int __cdecl AcosTableMovePrevRow(void* handle,
                                                         char* bufOut, int bufSize) {
    auto* core = ToCore(handle);
    if (!core || !core->reader) return -1;
    return CopyString(core->reader->TableMovePrevRow(), bufOut, bufSize);
}

} // extern "C"
