# src/Core/CMakeLists.txt
#
# Two targets:
#   AccessOSCore        — STATIC library — full C++ API, used by unit tests
#                         and by the DLL wrapper.
#   AccessOSCoreDll     — SHARED library — thin DLL that re-exports the C API
#                         surface (Bridge/AccessOSExports.cpp only) and links
#                         against AccessOSCore static. The WinUI 3 shell loads
#                         this via P/Invoke.

add_library(AccessOSCore STATIC
    # Error subsystem
    Error/AccessError.cpp

    # Logging subsystem
    Logging/Logger.cpp

    # UI Automation provider
    Accessibility/UIAutomation/UIAProvider.cpp

    # Event engine
    Events/EventQueue.cpp
    Events/UIAEventSink.cpp
    Events/EventEngine.cpp

    # Semantic model
    Semantic/SemanticCache.cpp
    Semantic/SemanticNormalizer.cpp
    Semantic/SemanticModel.cpp

    # Focus management
    Focus/FocusManager.cpp

    # Speech engine
    Speech/SpeechQueue.cpp
    Speech/SpeechFormatter.cpp
    Speech/SapiSpeechEngine.cpp
    Speech/SpeechManager.cpp

    # Commands & keyboard
    Commands/CommandManager.cpp
    Commands/ShortcutManager.cpp
    Commands/KeyboardManager.cpp

    # Navigation
    Navigation/NavigationEngine.cpp

    # Native messaging (browser extension bridge)
    Browser/NativeMessagingHost.cpp

    # NOTE: Bridge/AccessOSExports.cpp is NOT here — it lives in AccessOSCoreDll.

    # Settings subsystem
    Settings/SqliteSettingsStore.cpp
    Settings/SettingsManager.cpp

    # Privacy filter
    Privacy/PrivacyFilter.cpp

    # Context engine
    Context/ContextDetector.cpp
    Context/ContextEngine.cpp

    # Announcement engine
    Announcement/Announcement.cpp
    Announcement/AnnouncementEngine.cpp

    # Reader core
    Reader/AccessReader.cpp
    Reader/SayAll.cpp
    Reader/ClipboardReader.cpp

    # Diagnostics subsystem
    Diagnostics/DiagnosticsCounters.cpp
    Diagnostics/FileSink.cpp
    Diagnostics/DiagnosticsManager.cpp

    # Command registry
    Commands/CommandRegistry.cpp

    # Settings applier
    Settings/SettingsApplier.cpp

    # Braille output subsystem
    Braille/BrailleTranslator.cpp
    Braille/BrailleDisplayManager.cpp
    Braille/HidBrailleDisplay.cpp

    # OCR subsystem
    OCR/WinRtOcrEngine.cpp
    OCR/OcrManager.cpp

    # Adapter layer
    Adapters/UiaAdapter.cpp
    Adapters/BrowserAdapter.cpp
    Adapters/AdapterRegistry.cpp

    # Audio / Earcon subsystem
    Audio/WaveEarconEngine.cpp
    Audio/EarconManager.cpp

    # Table / Grid reading
    Table/TableNavigator.cpp

    # Browse mode / Virtual Cursor
    Browse/VirtualDocument.cpp
    Browse/VirtualCursor.cpp

    # Voice Profiles
    Speech/VoiceProfile.cpp

    # Per-application script rules
    Rules/AppRule.cpp

    # Localisation / i18n
    I18n/LocaleManager.cpp
)

target_include_directories(AccessOSCore
    PUBLIC
        "${CMAKE_SOURCE_DIR}/src/Core"
        "${CMAKE_BINARY_DIR}/generated"
)

target_link_libraries(AccessOSCore
    PUBLIC
        kernel32
        user32
        ole32        # COM
        oleaut32
        uuid
        uiautomationcore  # UI Automation
        sapi              # Windows Speech API
        sqlite3_lib       # SQLite amalgamation
        runtimeobject     # WinRT (Windows.Media.Ocr)
)

target_compile_options(AccessOSCore PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
)

# ─── AccessOSCoreDll — SHARED wrapper DLL (P/Invoke surface) ─────────────────
# Contains only the C export shim. Links the static lib so it contains all
# subsystem code. The WinUI 3 shell loads this via P/Invoke.

add_library(AccessOSCoreDll SHARED
    Bridge/AccessOSExports.cpp
)

target_link_libraries(AccessOSCoreDll
    PRIVATE
        AccessOSCore
)

target_include_directories(AccessOSCoreDll
    PRIVATE
        "${CMAKE_SOURCE_DIR}/src/Core"
        "${CMAKE_BINARY_DIR}/generated"
)

# Debug DLL gets the _d suffix so CoreBridge.cs can distinguish builds.
# ARCHIVE_OUTPUT_NAME controls the import lib name; we suffix with _dll to
# avoid colliding with the static AccessOSCore.lib in the same output dir.
set_target_properties(AccessOSCoreDll PROPERTIES
    OUTPUT_NAME_DEBUG          "AccessOSCore_d"
    OUTPUT_NAME_RELEASE        "AccessOSCore"
    OUTPUT_NAME_RELWITHDEBINFO "AccessOSCore"
    OUTPUT_NAME_MINSIZEREL     "AccessOSCore"
    ARCHIVE_OUTPUT_NAME_DEBUG          "AccessOSCoreDll_d"
    ARCHIVE_OUTPUT_NAME_RELEASE        "AccessOSCoreDll"
    ARCHIVE_OUTPUT_NAME_RELWITHDEBINFO "AccessOSCoreDll"
    ARCHIVE_OUTPUT_NAME_MINSIZEREL     "AccessOSCoreDll"
)

# Define the build-side export macro only for the DLL project.
target_compile_definitions(AccessOSCoreDll PRIVATE ACCESSOS_BUILDING_DLL)

target_compile_options(AccessOSCoreDll PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
)

# Post-build: copy DLL to the WinUI 3 app output directory.
set(UI_BIN_DIR "${CMAKE_SOURCE_DIR}/src/UI/bin/x64/$<CONFIG>/net8.0-windows10.0.26100.0/win-x64")

add_custom_command(TARGET AccessOSCoreDll POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${UI_BIN_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:AccessOSCoreDll>"
        "${UI_BIN_DIR}/$<TARGET_FILE_NAME:AccessOSCoreDll>"
    COMMENT "Copying AccessOSCore DLL → WinUI 3 output directory"
)

# ─── AccessOS diagnostic executable ──────────────────────────────────────────
# First milestone: native process that reads the focused UIA element.
add_executable(AccessOS
    Main.cpp
)

target_link_libraries(AccessOS
    PRIVATE
        AccessOSCore
)

target_include_directories(AccessOS
    PRIVATE
        "${CMAKE_SOURCE_DIR}/src/Core"
        "${CMAKE_BINARY_DIR}/generated"
)
