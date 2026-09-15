// UiaAdapter.cpp — UI Automation adapter (ACCESSOS-023)
#include "UiaAdapter.h"
#include <chrono>

namespace AccessOS {
namespace Adapters {

// ── InternalListener: converts AccessEvent → AdapterEvent ────────────────────

class UiaAdapter::InternalListener : public AccessOS::IEventListener {
public:
    explicit InternalListener(AdapterEventCallback* cb) : m_cb(cb) {}

    void OnEvent(const AccessOS::AccessEvent& event) override {
        if (!m_cb || !(*m_cb)) return;

        AdapterEvent ae;
        ae.sourceName  = "UIA";
        ae.elementName = event.element.name;
        ae.elementRole = std::to_string(static_cast<int>(event.element.role));
        ae.extraData   = event.extraInfo;

        // Map AccessEventType → AdapterEventKind
        using AET = AccessOS::AccessEventType;
        switch (event.type) {
        case AET::FocusChanged:
            ae.kind = AdapterEventKind::FocusChanged; break;
        case AET::StructureChanged:
            ae.kind = AdapterEventKind::StructureChange; break;
        case AET::NotificationRaised:
            ae.kind = AdapterEventKind::Alert; break;
        default:
            ae.kind = AdapterEventKind::PropertyChange; break;
        }

        // Timestamp: milliseconds since epoch
        ae.timestampMs = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count());

        (*m_cb)(ae);
    }

private:
    AdapterEventCallback* m_cb;
};

// ── UiaAdapter ────────────────────────────────────────────────────────────────

UiaAdapter::UiaAdapter(std::shared_ptr<AccessOS::EventEngine> engine)
    : m_engine(engine ? std::move(engine)
                      : std::make_shared<AccessOS::EventEngine>()) {}

UiaAdapter::~UiaAdapter() {
    Detach();
}

void UiaAdapter::SetEventCallback(AdapterEventCallback cb) {
    std::lock_guard<std::mutex> lk(m_mutex);
    m_callback = std::move(cb);
}

bool UiaAdapter::Attach() {
    std::lock_guard<std::mutex> lk(m_mutex);
    if (m_attached) return true;
    // Create listener pointing at our callback
    m_listener = std::make_shared<InternalListener>(&m_callback);
    m_engine->AddListener(m_listener.get());
    m_attached = true;
    return true;
}

void UiaAdapter::Detach() {
    std::lock_guard<std::mutex> lk(m_mutex);
    if (!m_attached) return;
    if (m_listener) {
        m_engine->RemoveListener(m_listener.get());
        m_listener.reset();
    }
    m_attached = false;
}

bool UiaAdapter::IsAttached() const {
    return m_attached.load();
}

} // namespace Adapters
} // namespace AccessOS
