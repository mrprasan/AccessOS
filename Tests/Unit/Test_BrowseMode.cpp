// Test_BrowseMode.cpp — ACCESSOS-026 Virtual Cursor / Browse Mode tests

#include <gtest/gtest.h>
#include "../../src/Core/Browse/VirtualNode.h"
#include "../../src/Core/Browse/VirtualDocument.h"
#include "../../src/Core/Browse/VirtualCursor.h"

using namespace AccessOS::Browse;

// ── Test document builder ─────────────────────────────────────────────────────

// Builds: [H1"Title"] [Text"Para 1"] [H2"Section"] [Text"Para 2"]
//         [Link"Click me"] [Button"Submit"] [Landmark"nav"]
//         [H3"Sub"] [TextInput"Name"] [H2"Footer"]
static std::shared_ptr<VirtualDocument> MakeTestDoc() {
    auto doc = std::make_shared<VirtualDocument>();
    auto add = [&](VirtualRole role, const std::string& text,
                   uint32_t hl = 0, const std::string& lm = "") {
        VirtualNode n;
        n.role         = role;
        n.text         = text;
        n.headingLevel = hl;
        n.landmarkLabel= lm;
        doc->AddNode(n);
    };
    add(VirtualRole::Heading1,  "Title",    1);
    add(VirtualRole::Text,      "Para 1");
    add(VirtualRole::Heading2,  "Section",  2);
    add(VirtualRole::Text,      "Para 2");
    add(VirtualRole::Link,      "Click me");
    add(VirtualRole::Button,    "Submit");
    add(VirtualRole::Landmark,  "nav",      0, "navigation");
    add(VirtualRole::Heading3,  "Sub",      3);
    add(VirtualRole::TextInput, "Name");
    add(VirtualRole::Heading2,  "Footer",   2);
    return doc;
}

// ── VirtualNode helpers ───────────────────────────────────────────────────────

TEST(VirtualNode, IsHeadingTrue) {
    EXPECT_TRUE(IsHeading(VirtualRole::Heading1));
    EXPECT_TRUE(IsHeading(VirtualRole::Heading3));
    EXPECT_TRUE(IsHeading(VirtualRole::Heading6));
}

TEST(VirtualNode, IsHeadingFalse) {
    EXPECT_FALSE(IsHeading(VirtualRole::Text));
    EXPECT_FALSE(IsHeading(VirtualRole::Link));
    EXPECT_FALSE(IsHeading(VirtualRole::Landmark));
}

TEST(VirtualNode, IsFormControlTrue) {
    EXPECT_TRUE(IsFormControl(VirtualRole::Button));
    EXPECT_TRUE(IsFormControl(VirtualRole::TextInput));
    EXPECT_TRUE(IsFormControl(VirtualRole::Checkbox));
}

TEST(VirtualNode, IsFormControlFalse) {
    EXPECT_FALSE(IsFormControl(VirtualRole::Text));
    EXPECT_FALSE(IsFormControl(VirtualRole::Heading1));
}

TEST(VirtualNode, IsLandmark) {
    EXPECT_TRUE(IsLandmark(VirtualRole::Landmark));
    EXPECT_FALSE(IsLandmark(VirtualRole::Text));
}

// ── VirtualDocument tests ─────────────────────────────────────────────────────

TEST(VirtualDocument, EmptyOnConstruct) {
    VirtualDocument doc;
    EXPECT_TRUE(doc.IsEmpty());
    EXPECT_EQ(doc.NodeCount(), 0u);
}

TEST(VirtualDocument, AddNodeIncreasesCount) {
    VirtualDocument doc;
    VirtualNode n; n.text = "hello"; n.role = VirtualRole::Text;
    doc.AddNode(n);
    EXPECT_EQ(doc.NodeCount(), 1u);
}

TEST(VirtualDocument, NodeIndexAssignedSequentially) {
    auto doc = MakeTestDoc();
    for (size_t i = 0; i < doc->NodeCount(); ++i) {
        EXPECT_EQ(doc->NodeAt(i).nodeIndex, static_cast<uint32_t>(i));
    }
}

TEST(VirtualDocument, NodeAtOutOfRangeThrows) {
    VirtualDocument doc;
    EXPECT_THROW(doc.NodeAt(0), std::out_of_range);
}

TEST(VirtualDocument, FullTextJoinsWithSpace) {
    VirtualDocument doc;
    VirtualNode a; a.text = "Hello"; doc.AddNode(a);
    VirtualNode b; b.text = "World"; doc.AddNode(b);
    EXPECT_EQ(doc.FullText(), "Hello World");
}

TEST(VirtualDocument, ClearEmptiesDocument) {
    auto doc = MakeTestDoc();
    doc->Clear();
    EXPECT_TRUE(doc->IsEmpty());
}

TEST(VirtualDocument, FindNextMatchesPredicate) {
    auto doc = MakeTestDoc();
    auto idx = doc->FindNext(0, [](const VirtualNode& n){ return n.role == VirtualRole::Link; });
    ASSERT_TRUE(idx.has_value());
    EXPECT_EQ(doc->NodeAt(*idx).text, "Click me");
}

TEST(VirtualDocument, FindNextNoMatchReturnsNullopt) {
    auto doc = MakeTestDoc();
    auto idx = doc->FindNext(0, [](const VirtualNode& n){ return n.role == VirtualRole::Table; });
    EXPECT_FALSE(idx.has_value());
}

TEST(VirtualDocument, FindPrevMatchesPredicate) {
    auto doc = MakeTestDoc();
    // Start from index 8 (TextInput), find prev link
    auto idx = doc->FindPrev(8, [](const VirtualNode& n){ return n.role == VirtualRole::Link; });
    ASSERT_TRUE(idx.has_value());
    EXPECT_EQ(doc->NodeAt(*idx).text, "Click me");
}

// ── VirtualCursor construction ────────────────────────────────────────────────

TEST(VirtualCursor, DefaultNoDocument) {
    VirtualCursor cur;
    EXPECT_FALSE(cur.HasDocument());
}

TEST(VirtualCursor, SetDocumentHasDocument) {
    VirtualCursor cur;
    cur.SetDocument(MakeTestDoc());
    EXPECT_TRUE(cur.HasDocument());
    EXPECT_EQ(cur.NodeIndex(), 0u);
}

TEST(VirtualCursor, ReadCurrentNodeAtStart) {
    VirtualCursor cur(MakeTestDoc());
    EXPECT_EQ(cur.ReadCurrentNode(), "Title");
}

// ── MoveTo ────────────────────────────────────────────────────────────────────

TEST(VirtualCursor, MoveToValidIndex) {
    VirtualCursor cur(MakeTestDoc());
    EXPECT_TRUE(cur.MoveTo(4));
    EXPECT_EQ(cur.ReadCurrentNode(), "Click me");
}

TEST(VirtualCursor, MoveToClampsOob) {
    VirtualCursor cur(MakeTestDoc());
    EXPECT_TRUE(cur.MoveTo(9999));
    EXPECT_EQ(cur.NodeIndex(), 9u); // last index in 10-node doc
}

TEST(VirtualCursor, MoveToStartAndEnd) {
    VirtualCursor cur(MakeTestDoc());
    cur.MoveTo(5);
    EXPECT_TRUE(cur.MoveToStart());
    EXPECT_EQ(cur.NodeIndex(), 0u);
    EXPECT_TRUE(cur.MoveToEnd());
    EXPECT_EQ(cur.NodeIndex(), 9u);
}

// ── Element navigation ────────────────────────────────────────────────────────

TEST(VirtualCursor, MoveNextElement) {
    VirtualCursor cur(MakeTestDoc());
    EXPECT_TRUE(cur.MoveNextElement());
    EXPECT_EQ(cur.NodeIndex(), 1u);
    EXPECT_EQ(cur.ReadCurrentNode(), "Para 1");
}

TEST(VirtualCursor, MovePrevElement) {
    VirtualCursor cur(MakeTestDoc());
    cur.MoveTo(3);
    EXPECT_TRUE(cur.MovePrevElement());
    EXPECT_EQ(cur.NodeIndex(), 2u);
}

TEST(VirtualCursor, MoveNextElementAtEndReturnsFalse) {
    VirtualCursor cur(MakeTestDoc());
    cur.MoveToEnd();
    EXPECT_FALSE(cur.MoveNextElement());
}

TEST(VirtualCursor, MovePrevElementAtStartReturnsFalse) {
    VirtualCursor cur(MakeTestDoc());
    EXPECT_FALSE(cur.MovePrevElement());
}

// ── Heading navigation ────────────────────────────────────────────────────────

TEST(VirtualCursor, MoveNextHeading) {
    VirtualCursor cur(MakeTestDoc());
    // At H1 "Title", next heading → H2 "Section"
    EXPECT_TRUE(cur.MoveNextHeading());
    EXPECT_EQ(cur.ReadCurrentNode(), "Section");
}

TEST(VirtualCursor, MoveNextHeadingChained) {
    VirtualCursor cur(MakeTestDoc());
    cur.MoveNextHeading(); // "Section"
    cur.MoveNextHeading(); // "Sub"
    EXPECT_EQ(cur.ReadCurrentNode(), "Sub");
}

TEST(VirtualCursor, MovePrevHeading) {
    VirtualCursor cur(MakeTestDoc());
    cur.MoveToEnd(); // at H2 "Footer"
    EXPECT_TRUE(cur.MovePrevHeading());
    EXPECT_EQ(cur.ReadCurrentNode(), "Sub");
}

TEST(VirtualCursor, MoveNextHeadingAtLastHeadingReturnsFalse) {
    VirtualCursor cur(MakeTestDoc());
    cur.MoveTo(9); // H2 "Footer" — last heading
    EXPECT_FALSE(cur.MoveNextHeading());
}

TEST(VirtualCursor, MoveNextHeadingLevel2) {
    VirtualCursor cur(MakeTestDoc());
    EXPECT_TRUE(cur.MoveNextHeadingLevel(2));
    EXPECT_EQ(cur.ReadCurrentNode(), "Section");
    EXPECT_TRUE(cur.MoveNextHeadingLevel(2));
    EXPECT_EQ(cur.ReadCurrentNode(), "Footer");
}

TEST(VirtualCursor, MovePrevHeadingLevel) {
    VirtualCursor cur(MakeTestDoc());
    cur.MoveToEnd();
    EXPECT_TRUE(cur.MovePrevHeadingLevel(2));
    EXPECT_EQ(cur.ReadCurrentNode(), "Section");
}

TEST(VirtualCursor, HeadingLevelNotFoundReturnsFalse) {
    VirtualCursor cur(MakeTestDoc());
    EXPECT_FALSE(cur.MoveNextHeadingLevel(6)); // no H6 in test doc
}

// ── Landmark navigation ───────────────────────────────────────────────────────

TEST(VirtualCursor, MoveNextLandmark) {
    VirtualCursor cur(MakeTestDoc());
    EXPECT_TRUE(cur.MoveNextLandmark());
    EXPECT_EQ(cur.ReadCurrentNode(), "nav");
}

TEST(VirtualCursor, MovePrevLandmark) {
    VirtualCursor cur(MakeTestDoc());
    cur.MoveToEnd();
    EXPECT_TRUE(cur.MovePrevLandmark());
    EXPECT_EQ(cur.ReadCurrentNode(), "nav");
}

TEST(VirtualCursor, NoLandmarkReturnsFalse) {
    // Document with no landmarks
    auto doc = std::make_shared<VirtualDocument>();
    VirtualNode n; n.text = "text"; n.role = VirtualRole::Text; doc->AddNode(n);
    VirtualCursor cur(doc);
    EXPECT_FALSE(cur.MoveNextLandmark());
}

// ── Form field navigation ─────────────────────────────────────────────────────

TEST(VirtualCursor, MoveNextFormField) {
    VirtualCursor cur(MakeTestDoc());
    EXPECT_TRUE(cur.MoveNextFormField()); // Button at index 5
    EXPECT_EQ(cur.ReadCurrentNode(), "Submit");
}

TEST(VirtualCursor, MoveNextFormFieldTwice) {
    VirtualCursor cur(MakeTestDoc());
    cur.MoveNextFormField(); // "Submit"
    EXPECT_TRUE(cur.MoveNextFormField()); // "Name"
    EXPECT_EQ(cur.ReadCurrentNode(), "Name");
}

// ── Link navigation ───────────────────────────────────────────────────────────

TEST(VirtualCursor, MoveNextLink) {
    VirtualCursor cur(MakeTestDoc());
    EXPECT_TRUE(cur.MoveNextLink());
    EXPECT_EQ(cur.ReadCurrentNode(), "Click me");
}

TEST(VirtualCursor, MovePrevLink) {
    VirtualCursor cur(MakeTestDoc());
    cur.MoveToEnd();
    EXPECT_TRUE(cur.MovePrevLink());
    EXPECT_EQ(cur.ReadCurrentNode(), "Click me");
}

// ── Announce position ─────────────────────────────────────────────────────────

TEST(VirtualCursor, AnnouncePositionFormat) {
    VirtualCursor cur(MakeTestDoc());
    std::string s = cur.AnnouncePosition();
    EXPECT_NE(s.find("node 1 of 10"), std::string::npos);
    EXPECT_NE(s.find("Title"),        std::string::npos);
}

TEST(VirtualCursor, AnnounceEmptyDocReturnsMessage) {
    VirtualCursor cur;
    EXPECT_EQ(cur.AnnouncePosition(), "Empty document");
}
