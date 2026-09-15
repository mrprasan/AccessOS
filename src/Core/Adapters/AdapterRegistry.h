#pragma once
// AdapterRegistry.h — owns and manages all IAccessAdapter instances (ACCESSOS-023)
//
// Responsibilities:
//   - Register / unregister adapters by name
//   - AttachAll / DetachAll lifecycle
//   - Route AdapterEvents from all adapters to a single pipeline callback
//   - Enable / disable individual adapters without detaching them
//   - Query adapter state

#include "IAccessAdapter.h"
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <mutex>
#include <unordered_map>

namespace AccessOS {
namespace Adapters {

using PipelineEventCallback = std::function<void(const AdapterEvent&)>;

class AdapterRegistry {
public:
    AdapterRegistry();

    // Set the callback that receives events from ALL adapters
    void SetPipelineCallback(PipelineEventCallback cb);

    // Register an adapter. Returns false if name already registered.
    bool Register(std::shared_ptr<IAccessAdapter> adapter);

    // Remove adapter by name (detaches first if attached)
    bool Unregister(const std::string& name);

    // Attach all registered adapters
    void AttachAll();

    // Detach all registered adapters
    void DetachAll();

    // Attach / detach a single adapter by name
    bool Attach(const std::string& name);
    bool Detach(const std::string& name);

    // Enable or disable routing of events from a given adapter
    // (adapter stays attached but its events are suppressed when disabled)
    void SetEnabled(const std::string& name, bool enabled);
    bool IsEnabled(const std::string& name) const;

    // Query
    bool IsAttached(const std::string& name) const;
    size_t Count() const;
    std::vector<std::string> Names() const;

    // Get a registered adapter by name (nullptr if not found)
    std::shared_ptr<IAccessAdapter> Get(const std::string& name) const;

private:
    struct Entry {
        std::shared_ptr<IAccessAdapter> adapter;
        bool enabled = true;
    };

    mutable std::mutex             m_mutex;
    std::vector<Entry>             m_entries;   // ordered list
    PipelineEventCallback          m_pipeline;

    Entry* FindEntry(const std::string& name);
    const Entry* FindEntry(const std::string& name) const;
};

} // namespace Adapters
} // namespace AccessOS
