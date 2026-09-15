// AdapterRegistry.cpp — owns and manages all IAccessAdapter instances (ACCESSOS-023)
#include "AdapterRegistry.h"
#include <algorithm>

namespace AccessOS {
namespace Adapters {

AdapterRegistry::AdapterRegistry() = default;

void AdapterRegistry::SetPipelineCallback(PipelineEventCallback cb) {
    std::lock_guard<std::mutex> lk(m_mutex);
    m_pipeline = std::move(cb);
}

bool AdapterRegistry::Register(std::shared_ptr<IAccessAdapter> adapter) {
    if (!adapter) return false;
    std::lock_guard<std::mutex> lk(m_mutex);
    const std::string name = adapter->GetName();
    // Duplicate check
    if (FindEntry(name)) return false;

    // Wrap the adapter's events through our pipeline, filtered by enabled flag
    auto* self = this;
    adapter->SetEventCallback([self, name](const AdapterEvent& ev) {
        std::lock_guard<std::mutex> cbLk(self->m_mutex);
        auto* e = self->FindEntry(name);
        if (!e || !e->enabled) return;
        if (self->m_pipeline) self->m_pipeline(ev);
    });

    m_entries.push_back({ std::move(adapter), true });
    return true;
}

bool AdapterRegistry::Unregister(const std::string& name) {
    std::lock_guard<std::mutex> lk(m_mutex);
    auto it = std::find_if(m_entries.begin(), m_entries.end(),
        [&](const Entry& e){ return e.adapter->GetName() == name; });
    if (it == m_entries.end()) return false;
    if (it->adapter->IsAttached()) it->adapter->Detach();
    m_entries.erase(it);
    return true;
}

void AdapterRegistry::AttachAll() {
    std::lock_guard<std::mutex> lk(m_mutex);
    for (auto& e : m_entries) {
        if (!e.adapter->IsAttached()) e.adapter->Attach();
    }
}

void AdapterRegistry::DetachAll() {
    std::lock_guard<std::mutex> lk(m_mutex);
    for (auto& e : m_entries) {
        if (e.adapter->IsAttached()) e.adapter->Detach();
    }
}

bool AdapterRegistry::Attach(const std::string& name) {
    std::lock_guard<std::mutex> lk(m_mutex);
    auto* e = FindEntry(name);
    if (!e) return false;
    return e->adapter->Attach();
}

bool AdapterRegistry::Detach(const std::string& name) {
    std::lock_guard<std::mutex> lk(m_mutex);
    auto* e = FindEntry(name);
    if (!e) return false;
    e->adapter->Detach();
    return true;
}

void AdapterRegistry::SetEnabled(const std::string& name, bool enabled) {
    std::lock_guard<std::mutex> lk(m_mutex);
    auto* e = FindEntry(name);
    if (e) e->enabled = enabled;
}

bool AdapterRegistry::IsEnabled(const std::string& name) const {
    std::lock_guard<std::mutex> lk(m_mutex);
    const auto* e = FindEntry(name);
    return e ? e->enabled : false;
}

bool AdapterRegistry::IsAttached(const std::string& name) const {
    std::lock_guard<std::mutex> lk(m_mutex);
    const auto* e = FindEntry(name);
    return e ? e->adapter->IsAttached() : false;
}

size_t AdapterRegistry::Count() const {
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_entries.size();
}

std::vector<std::string> AdapterRegistry::Names() const {
    std::lock_guard<std::mutex> lk(m_mutex);
    std::vector<std::string> out;
    out.reserve(m_entries.size());
    for (const auto& e : m_entries) out.push_back(e.adapter->GetName());
    return out;
}

std::shared_ptr<IAccessAdapter> AdapterRegistry::Get(const std::string& name) const {
    std::lock_guard<std::mutex> lk(m_mutex);
    const auto* e = FindEntry(name);
    return e ? e->adapter : nullptr;
}

// ── private ───────────────────────────────────────────────────────────────────

AdapterRegistry::Entry* AdapterRegistry::FindEntry(const std::string& name) {
    for (auto& e : m_entries) {
        if (e.adapter->GetName() == name) return &e;
    }
    return nullptr;
}

const AdapterRegistry::Entry* AdapterRegistry::FindEntry(const std::string& name) const {
    for (const auto& e : m_entries) {
        if (e.adapter->GetName() == name) return &e;
    }
    return nullptr;
}

} // namespace Adapters
} // namespace AccessOS
