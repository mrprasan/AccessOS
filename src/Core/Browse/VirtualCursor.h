#pragma once
// VirtualCursor.h — Browse mode virtual cursor (ACCESSOS-026)
//
// Maintains a position (node index + character offset within the node's text)
// and provides navigation commands.

#include "VirtualDocument.h"
#include <string>
#include <memory>

namespace AccessOS {
namespace Browse {

// Granularity for Move operations
enum class BrowseMoveUnit : uint8_t {
    Character = 0,
    Word      = 1,
    Line      = 2, // treated as one VirtualNode
    Element   = 3, // any node boundary
    Heading   = 4, // next/prev heading (any level)
    HeadingN  = 5, // next/prev heading at specific level
    Landmark  = 6, // next/prev landmark
    FormField = 7, // next/prev form control
    Link      = 8, // next/prev link
};

enum class BrowseMoveDirection : uint8_t { Forward = 0, Backward = 1 };

struct CursorPosition {
    size_t   nodeIndex  = 0;
    uint32_t charOffset = 0; // byte offset within node text (0 = before first char)
};

class VirtualCursor {
public:
    VirtualCursor();
    explicit VirtualCursor(std::shared_ptr<VirtualDocument> doc);

    void SetDocument(std::shared_ptr<VirtualDocument> doc);
    bool HasDocument() const noexcept;

    // Current position
    CursorPosition Position() const noexcept { return m_pos; }
    size_t NodeIndex()  const noexcept { return m_pos.nodeIndex; }
    uint32_t CharOffset() const noexcept { return m_pos.charOffset; }

    // Jump to explicit position (clamped to valid range)
    bool MoveTo(size_t nodeIndex, uint32_t charOffset = 0);

    // Move to document start / end
    bool MoveToStart();
    bool MoveToEnd();

    // Move by element (one node at a time)
    bool MoveNextElement();
    bool MovePrevElement();

    // Move to next/prev heading (any level)
    bool MoveNextHeading();
    bool MovePrevHeading();

    // Move to next/prev heading at a specific level (1–6)
    bool MoveNextHeadingLevel(uint32_t level);
    bool MovePrevHeadingLevel(uint32_t level);

    // Move to next/prev landmark
    bool MoveNextLandmark();
    bool MovePrevLandmark();

    // Move to next/prev form control
    bool MoveNextFormField();
    bool MovePrevFormField();

    // Move to next/prev link
    bool MoveNextLink();
    bool MovePrevLink();

    // Announce text at current position (full node text)
    std::string ReadCurrentNode() const;

    // Announce current position: "node N of M: <text>"
    std::string AnnouncePosition() const;

private:
    std::shared_ptr<VirtualDocument> m_doc;
    CursorPosition                   m_pos{};

    // Move forward/backward to the next node matching predicate
    bool MoveToNext(bool (*pred)(const VirtualNode&));
    bool MoveToPrev(bool (*pred)(const VirtualNode&));
};

} // namespace Browse
} // namespace AccessOS
