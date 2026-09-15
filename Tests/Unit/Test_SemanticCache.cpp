// Tests/Unit/Test_SemanticCache.cpp
// Unit tests for SemanticCache.

#include <gtest/gtest.h>
#include "Semantic/SemanticCache.h"

#include <thread>
#include <vector>
#include <atomic>

using namespace AccessOS;

static AccessNode MakeNode(uint64_t id, AccessRole role, const std::string& name,
                           uint64_t parentId = 0) {
    AccessNode n;
    n.id       = id;
    n.role     = role;
    n.name     = name;
    n.parentId = parentId;
    n.isValid  = true;
    return n;
}

// ─── Basic operations ─────────────────────────────────────────────────────────

TEST(SemanticCacheTest, EmptyCacheReturnsNullopt) {
    SemanticCache cache;
    EXPECT_FALSE(cache.Get(1).has_value());
    EXPECT_FALSE(cache.GetFocused().has_value());
    EXPECT_EQ(cache.Size(), 0u);
}

TEST(SemanticCacheTest, UpdateAndGetNode) {
    SemanticCache cache;
    cache.Update(MakeNode(1, AccessRole::Button, "Submit"));

    auto node = cache.Get(1);
    ASSERT_TRUE(node.has_value());
    EXPECT_EQ(node->name, "Submit");
    EXPECT_EQ(node->role, AccessRole::Button);
}

TEST(SemanticCacheTest, UpdateReplacesExistingNode) {
    SemanticCache cache;
    cache.Update(MakeNode(1, AccessRole::Button, "Submit"));
    cache.Update(MakeNode(1, AccessRole::Button, "Send"));   // same ID

    auto node = cache.Get(1);
    ASSERT_TRUE(node.has_value());
    EXPECT_EQ(node->name, "Send");
}

TEST(SemanticCacheTest, RemoveDeletesNode) {
    SemanticCache cache;
    cache.Update(MakeNode(1, AccessRole::Button, "X"));
    cache.Remove(1);

    EXPECT_FALSE(cache.Get(1).has_value());
    EXPECT_EQ(cache.Size(), 0u);
}

TEST(SemanticCacheTest, ClearRemovesAllNodes) {
    SemanticCache cache;
    cache.Update(MakeNode(1, AccessRole::Button, "A"));
    cache.Update(MakeNode(2, AccessRole::Edit,   "B"));
    cache.Clear();

    EXPECT_EQ(cache.Size(), 0u);
    EXPECT_FALSE(cache.GetFocused().has_value());
}

// ─── Focus ────────────────────────────────────────────────────────────────────

TEST(SemanticCacheTest, SetAndGetFocusedNode) {
    SemanticCache cache;
    cache.Update(MakeNode(42, AccessRole::Edit, "Username"));
    cache.SetFocusedId(42);

    auto focused = cache.GetFocused();
    ASSERT_TRUE(focused.has_value());
    EXPECT_EQ(focused->id,   42u);
    EXPECT_EQ(focused->name, "Username");
}

TEST(SemanticCacheTest, RemoveFocusedNodeClearsFocus) {
    SemanticCache cache;
    cache.Update(MakeNode(5, AccessRole::Button, "OK"));
    cache.SetFocusedId(5);
    cache.Remove(5);

    EXPECT_FALSE(cache.GetFocused().has_value());
}

TEST(SemanticCacheTest, ClearAlsoResetsFocus) {
    SemanticCache cache;
    cache.Update(MakeNode(7, AccessRole::Button, "Cancel"));
    cache.SetFocusedId(7);
    cache.Clear();

    EXPECT_FALSE(cache.GetFocused().has_value());
}

// ─── Children ────────────────────────────────────────────────────────────────

TEST(SemanticCacheTest, GetChildrenReturnsOnlyDirectChildren) {
    SemanticCache cache;
    cache.Update(MakeNode(1, AccessRole::Pane,   "Root",   0));
    cache.Update(MakeNode(2, AccessRole::Button, "Child1", 1));
    cache.Update(MakeNode(3, AccessRole::Button, "Child2", 1));
    cache.Update(MakeNode(4, AccessRole::Button, "Other",  99));

    auto children = cache.GetChildren(1);
    EXPECT_EQ(children.size(), 2u);
}

TEST(SemanticCacheTest, GetChildrenReturnsEmptyWhenNone) {
    SemanticCache cache;
    cache.Update(MakeNode(1, AccessRole::Button, "Lone"));
    EXPECT_TRUE(cache.GetChildren(1).empty());
}

// ─── Contains ────────────────────────────────────────────────────────────────

TEST(SemanticCacheTest, ContainsReturnsTrueForExistingNode) {
    SemanticCache cache;
    cache.Update(MakeNode(10, AccessRole::Button, "X"));
    EXPECT_TRUE(cache.Contains(10));
    EXPECT_FALSE(cache.Contains(99));
}

// ─── FindAll ─────────────────────────────────────────────────────────────────

TEST(SemanticCacheTest, FindAllMatchesByRole) {
    SemanticCache cache;
    cache.Update(MakeNode(1, AccessRole::Button,  "Btn1"));
    cache.Update(MakeNode(2, AccessRole::Button,  "Btn2"));
    cache.Update(MakeNode(3, AccessRole::Edit,    "Edit1"));
    cache.Update(MakeNode(4, AccessRole::CheckBox,"CB1"));

    auto buttons = cache.FindAll([](const AccessNode& n) {
        return n.role == AccessRole::Button;
    });
    EXPECT_EQ(buttons.size(), 2u);
}

TEST(SemanticCacheTest, FindAllReturnsEmptyWhenNoMatch) {
    SemanticCache cache;
    cache.Update(MakeNode(1, AccessRole::Button, "Btn"));

    auto links = cache.FindAll([](const AccessNode& n) {
        return n.role == AccessRole::Link;
    });
    EXPECT_TRUE(links.empty());
}

// ─── Thread safety ───────────────────────────────────────────────────────────

TEST(SemanticCacheTest, ConcurrentUpdatesAreThreadSafe) {
    SemanticCache cache;
    constexpr int kThreads = 4;
    constexpr int kNodes   = 50;

    std::vector<std::thread> writers;
    for (int t = 0; t < kThreads; ++t) {
        writers.emplace_back([&, t] {
            for (int i = 0; i < kNodes; ++i) {
                uint64_t id = static_cast<uint64_t>(t * kNodes + i + 1);
                cache.Update(MakeNode(id, AccessRole::Button, "n" + std::to_string(id)));
            }
        });
    }
    for (auto& w : writers) w.join();

    EXPECT_EQ(cache.Size(), static_cast<size_t>(kThreads * kNodes));
}
