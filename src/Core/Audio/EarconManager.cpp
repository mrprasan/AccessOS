// EarconManager.cpp — maps EarconId → ToneParams, drives IAudioEngine (ACCESSOS-024)
#include "EarconManager.h"
#include <algorithm>

namespace AccessOS {
namespace Audio {

EarconManager::EarconManager() {
    BuildDefaultTable();
}

EarconManager::EarconManager(std::shared_ptr<IAudioEngine> engine)
    : m_engine(std::move(engine)) {
    BuildDefaultTable();
}

void EarconManager::SetEngine(std::shared_ptr<IAudioEngine> engine) {
    std::lock_guard<std::mutex> lk(m_mutex);
    m_engine = std::move(engine);
}

std::shared_ptr<IAudioEngine> EarconManager::GetEngine() const {
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_engine;
}

bool EarconManager::IsAvailable() const {
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_engine && m_engine->IsAvailable();
}

bool EarconManager::Play(EarconId id) {
    if (id == EarconId::None) return false;
    if (!m_masterEnabled.load()) return false;

    std::lock_guard<std::mutex> lk(m_mutex);
    if (!m_engine) return false;

    // Per-earcon enabled check
    auto enIt = m_enabledMap.find(static_cast<uint32_t>(id));
    if (enIt != m_enabledMap.end() && !enIt->second) return false;

    // Look up tone params
    auto it = m_toneTable.find(static_cast<uint32_t>(id));
    ToneParams params = (it != m_toneTable.end()) ? it->second : DefaultTone(id);

    return m_engine->PlayTone(params);
}

void EarconManager::SetToneParams(EarconId id, const ToneParams& params) {
    std::lock_guard<std::mutex> lk(m_mutex);
    m_toneTable[static_cast<uint32_t>(id)] = params;
}

ToneParams EarconManager::GetToneParams(EarconId id) const {
    std::lock_guard<std::mutex> lk(m_mutex);
    auto it = m_toneTable.find(static_cast<uint32_t>(id));
    return (it != m_toneTable.end()) ? it->second : DefaultTone(id);
}

void EarconManager::SetEarconEnabled(EarconId id, bool enabled) {
    std::lock_guard<std::mutex> lk(m_mutex);
    m_enabledMap[static_cast<uint32_t>(id)] = enabled;
}

bool EarconManager::IsEarconEnabled(EarconId id) const {
    std::lock_guard<std::mutex> lk(m_mutex);
    auto it = m_enabledMap.find(static_cast<uint32_t>(id));
    return (it == m_enabledMap.end()) ? true : it->second;
}

void EarconManager::SetEnabled(bool enabled) {
    m_masterEnabled.store(enabled);
}

bool EarconManager::IsEnabled() const {
    return m_masterEnabled.load();
}

void EarconManager::SetVolume(float volume) {
    std::lock_guard<std::mutex> lk(m_mutex);
    if (m_engine) m_engine->SetVolume(volume);
}

float EarconManager::GetVolume() const {
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_engine ? m_engine->GetVolume() : 0.0f;
}

void EarconManager::SetMuted(bool muted) {
    std::lock_guard<std::mutex> lk(m_mutex);
    if (m_engine) m_engine->SetMuted(muted);
}

bool EarconManager::IsMuted() const {
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_engine ? m_engine->IsMuted() : false;
}

// ── private ───────────────────────────────────────────────────────────────────

void EarconManager::BuildDefaultTable() {
    // Populate the default tone table for all known EarconIds
    static constexpr EarconId kAll[] = {
        EarconId::FocusEnter, EarconId::FocusLeave,
        EarconId::MenuItem,   EarconId::ListItem,
        EarconId::Link,       EarconId::Button,
        EarconId::Checkbox,   EarconId::Combobox,   EarconId::TextInput,
        EarconId::Checked,    EarconId::Unchecked,
        EarconId::Expanded,   EarconId::Collapsed,  EarconId::Selected,
        EarconId::Alert,      EarconId::Error,
        EarconId::Warning,    EarconId::Success,
        EarconId::LineStart,  EarconId::LineEnd,
        EarconId::DocumentStart, EarconId::DocumentEnd,
        EarconId::Boundary,
    };
    for (EarconId id : kAll) {
        m_toneTable[static_cast<uint32_t>(id)] = DefaultTone(id);
        m_enabledMap[static_cast<uint32_t>(id)] = true;
    }
}

/*static*/
ToneParams EarconManager::DefaultTone(EarconId id) {
    // Each earcon has a distinct frequency + duration so they are perceptually
    // distinguishable. Frequencies follow the piano scale pattern with some
    // deliberate steps for auditory distinctiveness.
    ToneParams p;
    p.fadeInMs  = 5.0f;
    p.fadeOutMs = 10.0f;
    p.volume    = 0.6f;

    switch (id) {
    // Focus & navigation — mid-range, short
    case EarconId::FocusEnter:    p.frequencyHz = 880.0f;  p.durationMs = 60.0f;  break;
    case EarconId::FocusLeave:    p.frequencyHz = 660.0f;  p.durationMs = 50.0f;  break;
    case EarconId::MenuItem:      p.frequencyHz = 784.0f;  p.durationMs = 60.0f;  break;
    case EarconId::ListItem:      p.frequencyHz = 740.0f;  p.durationMs = 55.0f;  break;
    case EarconId::Link:          p.frequencyHz = 1047.0f; p.durationMs = 65.0f;  break;
    case EarconId::Button:        p.frequencyHz = 932.0f;  p.durationMs = 60.0f;  break;
    case EarconId::Checkbox:      p.frequencyHz = 698.0f;  p.durationMs = 60.0f;  break;
    case EarconId::Combobox:      p.frequencyHz = 830.0f;  p.durationMs = 60.0f;  break;
    case EarconId::TextInput:     p.frequencyHz = 587.0f;  p.durationMs = 60.0f;  break;
    // State changes — slightly longer to convey toggle
    case EarconId::Checked:       p.frequencyHz = 1175.0f; p.durationMs = 80.0f;  break;
    case EarconId::Unchecked:     p.frequencyHz = 523.0f;  p.durationMs = 80.0f;  break;
    case EarconId::Expanded:      p.frequencyHz = 988.0f;  p.durationMs = 70.0f;  break;
    case EarconId::Collapsed:     p.frequencyHz = 622.0f;  p.durationMs = 70.0f;  break;
    case EarconId::Selected:      p.frequencyHz = 1046.0f; p.durationMs = 65.0f;  break;
    // Alerts — louder, longer
    case EarconId::Alert:         p.frequencyHz = 1320.0f; p.durationMs = 120.0f; p.volume = 0.8f; break;
    case EarconId::Error:         p.frequencyHz = 220.0f;  p.durationMs = 150.0f; p.volume = 0.8f; break;
    case EarconId::Warning:       p.frequencyHz = 440.0f;  p.durationMs = 120.0f; p.volume = 0.75f; break;
    case EarconId::Success:       p.frequencyHz = 1568.0f; p.durationMs = 100.0f; p.volume = 0.7f; break;
    // Reading boundaries — subtle
    case EarconId::LineStart:     p.frequencyHz = 523.0f;  p.durationMs = 40.0f;  p.volume = 0.4f; break;
    case EarconId::LineEnd:       p.frequencyHz = 494.0f;  p.durationMs = 40.0f;  p.volume = 0.4f; break;
    case EarconId::DocumentStart: p.frequencyHz = 659.0f;  p.durationMs = 80.0f;  p.volume = 0.5f; break;
    case EarconId::DocumentEnd:   p.frequencyHz = 494.0f;  p.durationMs = 80.0f;  p.volume = 0.5f; break;
    case EarconId::Boundary:      p.frequencyHz = 330.0f;  p.durationMs = 60.0f;  p.volume = 0.5f; break;
    default:
        p.frequencyHz = 440.0f;
        p.durationMs  = 80.0f;
        break;
    }
    return p;
}

} // namespace Audio
} // namespace AccessOS
