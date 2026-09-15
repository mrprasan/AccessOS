#pragma once
// BrowserAdapter.h — Native Messaging / browser extension adapter (ACCESSOS-023)
//
// Wraps NativeMessagingHost behind IAccessAdapter.
// On Attach(), registers internal callbacks on NativeMessagingHost for:
//   - Browser focus change events  → AdapterEventKind::BrowserFocus
//   - Page load events             → AdapterEventKind::BrowserPageLoad

#include "IAccessAdapter.h"
#include "../Browser/NativeMessagingHost.h"
#include <memory>
#include <atomic>
#include <mutex>

namespace AccessOS {
namespace Adapters {

class BrowserAdapter : public IAccessAdapter {
public:
    explicit BrowserAdapter(std::shared_ptr<NativeMessagingHost> host = nullptr);
    ~BrowserAdapter() override;

    std::string GetName() const override { return "Browser"; }
    void SetEventCallback(AdapterEventCallback cb) override;
    bool Attach()  override;
    void Detach()  override;
    bool IsAttached() const override;

private:
    std::shared_ptr<NativeMessagingHost> m_host;
    AdapterEventCallback                 m_callback;
    std::atomic<bool>                    m_attached{ false };
    mutable std::mutex                   m_mutex;

    void OnBrowserFocus(const std::string& name, const std::string& role);
    void OnBrowserPageLoad(const std::string& url, const std::string& title);
};

} // namespace Adapters
} // namespace AccessOS
