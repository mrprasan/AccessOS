// BrowserAdapter.cpp — Native Messaging / browser extension adapter (ACCESSOS-023)
#include "BrowserAdapter.h"
#include <chrono>

namespace AccessOS {
namespace Adapters {

BrowserAdapter::BrowserAdapter(std::shared_ptr<NativeMessagingHost> host)
    : m_host(std::move(host)) {}

BrowserAdapter::~BrowserAdapter() {
    Detach();
}

void BrowserAdapter::SetEventCallback(AdapterEventCallback cb) {
    std::lock_guard<std::mutex> lk(m_mutex);
    m_callback = std::move(cb);
}

bool BrowserAdapter::Attach() {
    std::lock_guard<std::mutex> lk(m_mutex);
    if (m_attached) return true;

    if (!m_host) {
        // Create a NativeMessagingHost that routes its events through our callback
        m_host = std::make_shared<NativeMessagingHost>(
            [this](const AccessEvent& event) {
                AdapterEvent ae;
                ae.sourceName  = "Browser";
                ae.elementName = event.element.name;
                ae.elementRole = std::to_string(static_cast<int>(event.element.role));
                ae.extraData   = event.extraInfo;
                ae.timestampMs = static_cast<uint64_t>(
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now().time_since_epoch()).count());

                if (event.type == AccessEventType::FocusChanged) {
                    ae.kind = AdapterEventKind::BrowserFocus;
                } else {
                    ae.kind = AdapterEventKind::BrowserPageLoad;
                }

                std::lock_guard<std::mutex> cbLk(m_mutex);
                if (m_callback) m_callback(ae);
            });
    }

    m_attached = true;
    return true;
}

void BrowserAdapter::Detach() {
    std::lock_guard<std::mutex> lk(m_mutex);
    if (!m_attached) return;
    if (m_host && m_host->IsRunning()) {
        m_host->Stop();
    }
    m_attached = false;
}

bool BrowserAdapter::IsAttached() const {
    return m_attached.load();
}

void BrowserAdapter::OnBrowserFocus(const std::string& name,
                                     const std::string& role) {
    std::lock_guard<std::mutex> lk(m_mutex);
    if (!m_callback) return;
    AdapterEvent ae;
    ae.kind        = AdapterEventKind::BrowserFocus;
    ae.sourceName  = "Browser";
    ae.elementName = name;
    ae.elementRole = role;
    ae.timestampMs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    m_callback(ae);
}

void BrowserAdapter::OnBrowserPageLoad(const std::string& url,
                                        const std::string& title) {
    std::lock_guard<std::mutex> lk(m_mutex);
    if (!m_callback) return;
    AdapterEvent ae;
    ae.kind        = AdapterEventKind::BrowserPageLoad;
    ae.sourceName  = "Browser";
    ae.elementName = title;
    ae.extraData   = url;
    ae.timestampMs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    m_callback(ae);
}

} // namespace Adapters
} // namespace AccessOS
