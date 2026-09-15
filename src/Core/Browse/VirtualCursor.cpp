// VirtualCursor.cpp — Browse mode virtual cursor (ACCESSOS-026)
#include "VirtualCursor.h"
#include <sstream>
#include <algorithm>

namespace AccessOS {
namespace Browse {

VirtualCursor::VirtualCursor() = default;

VirtualCursor::VirtualCursor(std::shared_ptr<VirtualDocument> doc)
    : m_doc(std::move(doc)) {}

void VirtualCursor::SetDocument(std::shared_ptr<VirtualDocument> doc) {
    m_doc = std::move(doc);
    m_pos = {};
}

bool VirtualCursor::HasDocument() const noexcept {
    return m_doc && !m_doc->IsEmpty();
}

bool VirtualCursor::MoveTo(size_t nodeIndex, uint32_t charOffset) {
    if (!m_doc || m_doc->IsEmpty()) return false;
    m_pos.nodeIndex  = std::min(nodeIndex, m_doc->NodeCount() - 1);
    const auto& node = m_doc->NodeAt(m_pos.nodeIndex);
    m_pos.charOffset = static_cast<uint32_t>(
        std::min(static_cast<size_t>(charOffset), node.text.size()));
    return true;
}

bool VirtualCursor::MoveToStart() {
    if (!m_doc || m_doc->IsEmpty()) return false;
    m_pos = {};
    return true;
}

bool VirtualCursor::MoveToEnd() {
    if (!m_doc || m_doc->IsEmpty()) return false;
    m_pos.nodeIndex  = m_doc->NodeCount() - 1;
    m_pos.charOffset = static_cast<uint32_t>(
        m_doc->NodeAt(m_pos.nodeIndex).text.size());
    return true;
}

bool VirtualCursor::MoveNextElement() {
    if (!m_doc || m_doc->IsEmpty()) return false;
    if (m_pos.nodeIndex + 1 >= m_doc->NodeCount()) return false;
    ++m_pos.nodeIndex;
    m_pos.charOffset = 0;
    return true;
}

bool VirtualCursor::MovePrevElement() {
    if (!m_doc || m_doc->IsEmpty()) return false;
    if (m_pos.nodeIndex == 0) return false;
    --m_pos.nodeIndex;
    m_pos.charOffset = 0;
    return true;
}

bool VirtualCursor::MoveNextHeading() {
    return MoveToNext([](const VirtualNode& n){ return IsHeading(n.role); });
}

bool VirtualCursor::MovePrevHeading() {
    return MoveToPrev([](const VirtualNode& n){ return IsHeading(n.role); });
}

bool VirtualCursor::MoveNextHeadingLevel(uint32_t level) {
    // Capture level in a thread-local static to pass through C-function-pointer predicate
    // Instead use a linear scan here (no lambda with capture through function pointer)
    if (!m_doc || m_doc->IsEmpty()) return false;
    for (size_t i = m_pos.nodeIndex + 1; i < m_doc->NodeCount(); ++i) {
        const auto& node = m_doc->NodeAt(i);
        if (IsHeading(node.role) && node.headingLevel == level) {
            m_pos.nodeIndex  = i;
            m_pos.charOffset = 0;
            return true;
        }
    }
    return false;
}

bool VirtualCursor::MovePrevHeadingLevel(uint32_t level) {
    if (!m_doc || m_doc->IsEmpty() || m_pos.nodeIndex == 0) return false;
    for (size_t i = m_pos.nodeIndex - 1; ; --i) {
        const auto& node = m_doc->NodeAt(i);
        if (IsHeading(node.role) && node.headingLevel == level) {
            m_pos.nodeIndex  = i;
            m_pos.charOffset = 0;
            return true;
        }
        if (i == 0) break;
    }
    return false;
}

bool VirtualCursor::MoveNextLandmark() {
    return MoveToNext([](const VirtualNode& n){ return IsLandmark(n.role); });
}

bool VirtualCursor::MovePrevLandmark() {
    return MoveToPrev([](const VirtualNode& n){ return IsLandmark(n.role); });
}

bool VirtualCursor::MoveNextFormField() {
    return MoveToNext([](const VirtualNode& n){ return IsFormControl(n.role); });
}

bool VirtualCursor::MovePrevFormField() {
    return MoveToPrev([](const VirtualNode& n){ return IsFormControl(n.role); });
}

bool VirtualCursor::MoveNextLink() {
    return MoveToNext([](const VirtualNode& n){ return n.role == VirtualRole::Link; });
}

bool VirtualCursor::MovePrevLink() {
    return MoveToPrev([](const VirtualNode& n){ return n.role == VirtualRole::Link; });
}

std::string VirtualCursor::ReadCurrentNode() const {
    if (!m_doc || m_doc->IsEmpty()) return "";
    return m_doc->NodeAt(m_pos.nodeIndex).text;
}

std::string VirtualCursor::AnnouncePosition() const {
    if (!m_doc || m_doc->IsEmpty()) return "Empty document";
    std::ostringstream ss;
    ss << "node " << (m_pos.nodeIndex + 1)
       << " of " << m_doc->NodeCount()
       << ": " << m_doc->NodeAt(m_pos.nodeIndex).text;
    return ss.str();
}

// ── private ───────────────────────────────────────────────────────────────────

bool VirtualCursor::MoveToNext(bool (*pred)(const VirtualNode&)) {
    if (!m_doc || m_doc->IsEmpty()) return false;
    auto found = m_doc->FindNext(m_pos.nodeIndex + 1, pred);
    if (!found) return false;
    m_pos.nodeIndex  = *found;
    m_pos.charOffset = 0;
    return true;
}

bool VirtualCursor::MoveToPrev(bool (*pred)(const VirtualNode&)) {
    if (!m_doc || m_doc->IsEmpty() || m_pos.nodeIndex == 0) return false;
    auto found = m_doc->FindPrev(m_pos.nodeIndex - 1, pred);
    if (!found) return false;
    m_pos.nodeIndex  = *found;
    m_pos.charOffset = 0;
    return true;
}

} // namespace Browse
} // namespace AccessOS
