#pragma once
// UiaAdapter.h — UI Automation adapter (ACCESSOS-023)
//
// Wraps the AccessOS UIAProvider + EventEngine behind IAccessAdapter.
// On Attach(), starts the EventEngine and registers an internal IEventListener
// that converts AccessEvents → AdapterEvents and forwards them via the callback.

#include "IAccessAdapter.h"
#include "../Events/EventEngine.h"
#include "../Events/IEventListener.h"
#include <memory>
#include <atomic>
#include <mutex>

namespace AccessOS {
namespace Adapters {

class UiaAdapter : public IAccessAdapter {
public:
    // engine may be nullptr; adapter will create its own EventEngine if so.
    explicit UiaAdapter(std::shared_ptr<AccessOS::EventEngine> engine = nullptr);
    ~UiaAdapter() override;

    std::string GetName() const override { return "UIA"; }
    void SetEventCallback(AdapterEventCallback cb) override;
    bool Attach()  override;
    void Detach()  override;
    bool IsAttached() const override;

private:
    std::shared_ptr<AccessOS::EventEngine> m_engine;
    AdapterEventCallback                   m_callback;
    std::atomic<bool>                      m_attached{ false };
    mutable std::mutex                     m_mutex;

    // Internal listener registered with EventEngine
    class InternalListener;
    std::shared_ptr<InternalListener> m_listener;
};

} // namespace Adapters
} // namespace AccessOS
