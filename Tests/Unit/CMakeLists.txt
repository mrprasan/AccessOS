# Tests/Unit/CMakeLists.txt
# Unit tests for AccessOS core components.

add_executable(AccessOSUnitTests
    Test_AccessError.cpp
    Test_Logger.cpp
    Test_AccessNode.cpp
    Test_UIARoleMap.cpp
    Test_EventQueue.cpp
    Test_EventEngine.cpp
    Test_SemanticCache.cpp
    Test_SemanticNormalizer.cpp
    Test_FocusManager.cpp
    Test_SpeechFormatter.cpp
    Test_CommandManager.cpp
    Test_ShortcutManager.cpp
    Test_NavigationEngine.cpp
    Test_SettingsManager.cpp
    Test_PrivacyFilter.cpp
    Test_ContextDetector.cpp
    Test_ContextEngine.cpp
    Test_AnnouncementEngine.cpp
    Test_AccessReader.cpp
    Test_DiagnosticsManager.cpp
    Test_CommandRegistry.cpp
    Test_SettingsApplier.cpp
    Test_BrailleSubsystem.cpp
    Test_OcrSubsystem.cpp
    Test_AdapterLayer.cpp
    Test_AudioSubsystem.cpp
    Test_TableReading.cpp
    Test_BrowseMode.cpp
    Test_Phase2Features.cpp
    Test_HidBrailleDriver.cpp
    Test_VoiceProfiles.cpp
    Test_AppRules.cpp
    Test_Integration.cpp
)

target_link_libraries(AccessOSUnitTests
    PRIVATE
        AccessOSCore
        GTest::gtest_main
)

target_include_directories(AccessOSUnitTests
    PRIVATE
        "${CMAKE_SOURCE_DIR}/src/Core"
        "${CMAKE_BINARY_DIR}/generated"
)

gtest_discover_tests(AccessOSUnitTests)
