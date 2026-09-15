// AccessOS/src/Core/Browser/NativeMessagingHost.h
//
// NativeMessagingHost — reads Chrome/Edge native messaging framing from stdin,
// deserialises JSON messages from the browser extension, converts them to
// AccessEvents, and injects them into the EventEngine queue.
//
// Protocol: https://developer.chrome.com/docs/extensions/mv3/nativeMessaging/
//   - Each message is prefixed by a 4-byte little-endian unsigned int (message length).
//   - The message body is a UTF-8 JSON string.
//   - Responses are written to stdout with the same framing.
//
// Why: The browser extension cannot call the C++ core directly.
//      The native messaging host is the bridge that runs as a separate
//      process (or in-process via stdin redirect) and feeds browser
//      accessibility events into the core pipeline.
//
// Threading:
//   - Run() blocks on stdin reads — call from a dedicated thread.
//   - Stop() signals the read loop to exit.
//   - EventEngine injection is thread-safe.
//
// Must NOT:
//   - Log passwords, PINs, auth tokens.
//   - Block the EventEngine worker thread.

#pragma once

#include "../Events/IEventListener.h"
#include "../Events/EventQueue.h"
#include "../Semantic/AccessNode.h"

#include <string>
#include <atomic>
#include <functional>

namespace AccessOS {

/// Callback invoked when a parsed AccessEvent is ready.
/// Signature: void(const AccessEvent&)
using BrowserEventCallback = std::function<void(const AccessEvent&)>;

class NativeMessagingHost {
public:
    explicit NativeMessagingHost(BrowserEventCallback callback);
    ~NativeMessagingHost() = default;

    /// Block-reads native messaging frames from stdin until Stop() is called.
    /// Returns when stdin is closed or Stop() is called.
    void Run();

    /// Signal the Run() loop to exit cleanly.
    void Stop() noexcept;

    /// Returns true if the host is actively reading.
    bool IsRunning() const noexcept;

    /// Write a response message to stdout (native messaging framing).
    /// Thread-safe.
    static void WriteResponse(const std::string& jsonResponse);

private:
    /// Read exactly `size` bytes from stdin. Returns false on EOF/error.
    static bool ReadExact(char* buf, uint32_t size);

    /// Read one complete native message from stdin.
    /// Returns the JSON body, or empty string on EOF/error.
    static std::string ReadMessage();

    /// Parse one JSON message and build an AccessEvent.
    /// Returns nullopt if the message is not a recognized type.
    static std::optional<AccessEvent> ParseMessage(const std::string& json);

    /// Parse a FOCUS_CHANGED payload.
    static std::optional<AccessEvent> ParseFocusChanged(const std::string& json);

    /// Parse a PAGE_LOADED payload.
    static std::optional<AccessEvent> ParsePageLoaded(const std::string& json);

    /// Map a browser role string to AccessRole.
    static AccessRole BrowserRoleToAccessRole(const std::string& role) noexcept;

    BrowserEventCallback    m_callback;
    std::atomic<bool>       m_running{ false };
};

} // namespace AccessOS
