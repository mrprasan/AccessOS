// Test_AdapterLayer.cpp — ACCESSOS-023 Adapter Layer tests
// Covers: AdapterEvent model, AdapterRegistry lifecycle, enable/disable routing

#include <gtest/gtest.h>
#include "../../src/Core/Adapters/IAccessAdapter.h"
#include "../../src/Core/Adapters/AdapterRegistry.h"

using namespace AccessOS::Adapters;

// ── Stub adapter ─────────────────────────────────────────────────────────────

class StubAdapter : public IAccessAdapter {
public:
    explicit StubAdapter(std::string name) : m_name(std::move(name)) {}

    std::string GetName() const override { return m_name; }

    void SetEventCallback(AdapterEventCallback cb) override {
        m_callback = std::move(cb);
    }

    bool Attach() override {
        m_attached = true;
        ++m_attachCount;
        return true;
    }

    void Detach() override {
        m_attached = false;
        ++m_detachCount;
    }

    bool IsAttached() const override { return m_attached; }

    // Test helper: fire an event via the registered callback
    void FireEvent(AdapterEventKind kind, const std::string& elemName = "") {
        if (!m_callback) return;
        AdapterEvent ev;
        ev.kind        = kind;
        ev.sourceName  = m_name;
        ev.elementName = elemName;
        ev.timestampMs = 42;
        m_callback(ev);
    }

    int AttachCount() const { return m_attachCount; }
    int DetachCount() const { return m_detachCount; }

private:
    std::string          m_name;
    AdapterEventCallback m_callback;
    bool                 m_attached    = false;
    int                  m_attachCount = 0;
    int                  m_detachCount = 0;
};

// ── AdapterEvent tests ────────────────────────────────────────────────────────

TEST(AdapterEvent, DefaultValues) {
    AdapterEvent ev;
    EXPECT_EQ(ev.kind, AdapterEventKind::FocusChanged);
    EXPECT_TRUE(ev.sourceName.empty());
    EXPECT_EQ(ev.timestampMs, 0u);
}

TEST(AdapterEvent, AllKindsDistinct) {
    // Ensure enum values are distinct
    EXPECT_NE(static_cast<int>(AdapterEventKind::FocusChanged),
              static_cast<int>(AdapterEventKind::BrowserFocus));
    EXPECT_NE(static_cast<int>(AdapterEventKind::BrowserFocus),
              static_cast<int>(AdapterEventKind::BrowserPageLoad));
    EXPECT_NE(static_cast<int>(AdapterEventKind::PropertyChange),
              static_cast<int>(AdapterEventKind::Alert));
}

// ── AdapterRegistry basic tests ──────────────────────────────────────────────

TEST(AdapterRegistry, EmptyOnConstruct) {
    AdapterRegistry reg;
    EXPECT_EQ(reg.Count(), 0u);
    EXPECT_TRUE(reg.Names().empty());
}

TEST(AdapterRegistry, RegisterOneAdapter) {
    AdapterRegistry reg;
    EXPECT_TRUE(reg.Register(std::make_shared<StubAdapter>("UIA")));
    EXPECT_EQ(reg.Count(), 1u);
}

TEST(AdapterRegistry, RegisterDuplicateNameFails) {
    AdapterRegistry reg;
    reg.Register(std::make_shared<StubAdapter>("UIA"));
    EXPECT_FALSE(reg.Register(std::make_shared<StubAdapter>("UIA")));
    EXPECT_EQ(reg.Count(), 1u);
}

TEST(AdapterRegistry, RegisterNullptrFails) {
    AdapterRegistry reg;
    EXPECT_FALSE(reg.Register(nullptr));
}

TEST(AdapterRegistry, RegisterMultiple) {
    AdapterRegistry reg;
    reg.Register(std::make_shared<StubAdapter>("UIA"));
    reg.Register(std::make_shared<StubAdapter>("Browser"));
    EXPECT_EQ(reg.Count(), 2u);
}

TEST(AdapterRegistry, NamesReturnedInOrder) {
    AdapterRegistry reg;
    reg.Register(std::make_shared<StubAdapter>("UIA"));
    reg.Register(std::make_shared<StubAdapter>("Browser"));
    auto names = reg.Names();
    ASSERT_EQ(names.size(), 2u);
    EXPECT_EQ(names[0], "UIA");
    EXPECT_EQ(names[1], "Browser");
}

TEST(AdapterRegistry, UnregisterByName) {
    AdapterRegistry reg;
    reg.Register(std::make_shared<StubAdapter>("UIA"));
    EXPECT_TRUE(reg.Unregister("UIA"));
    EXPECT_EQ(reg.Count(), 0u);
}

TEST(AdapterRegistry, UnregisterNonExistentReturnsFalse) {
    AdapterRegistry reg;
    EXPECT_FALSE(reg.Unregister("NoSuchAdapter"));
}

TEST(AdapterRegistry, GetByName) {
    AdapterRegistry reg;
    auto stub = std::make_shared<StubAdapter>("UIA");
    reg.Register(stub);
    EXPECT_EQ(reg.Get("UIA"), stub);
}

TEST(AdapterRegistry, GetUnknownReturnsNull) {
    AdapterRegistry reg;
    EXPECT_EQ(reg.Get("Missing"), nullptr);
}

// ── Attach / Detach tests ─────────────────────────────────────────────────────

TEST(AdapterRegistry, AttachAllCallsAttachOnAll) {
    AdapterRegistry reg;
    auto s1 = std::make_shared<StubAdapter>("A");
    auto s2 = std::make_shared<StubAdapter>("B");
    reg.Register(s1);
    reg.Register(s2);
    reg.AttachAll();
    EXPECT_TRUE(s1->IsAttached());
    EXPECT_TRUE(s2->IsAttached());
}

TEST(AdapterRegistry, DetachAllCallsDetachOnAll) {
    AdapterRegistry reg;
    auto s1 = std::make_shared<StubAdapter>("A");
    auto s2 = std::make_shared<StubAdapter>("B");
    reg.Register(s1);
    reg.Register(s2);
    reg.AttachAll();
    reg.DetachAll();
    EXPECT_FALSE(s1->IsAttached());
    EXPECT_FALSE(s2->IsAttached());
}

TEST(AdapterRegistry, AttachByName) {
    AdapterRegistry reg;
    auto stub = std::make_shared<StubAdapter>("UIA");
    reg.Register(stub);
    EXPECT_TRUE(reg.Attach("UIA"));
    EXPECT_TRUE(stub->IsAttached());
}

TEST(AdapterRegistry, AttachUnknownReturnsFalse) {
    AdapterRegistry reg;
    EXPECT_FALSE(reg.Attach("Unknown"));
}

TEST(AdapterRegistry, DetachByName) {
    AdapterRegistry reg;
    auto stub = std::make_shared<StubAdapter>("UIA");
    reg.Register(stub);
    reg.Attach("UIA");
    EXPECT_TRUE(reg.Detach("UIA"));
    EXPECT_FALSE(stub->IsAttached());
}

TEST(AdapterRegistry, DetachUnknownReturnsFalse) {
    AdapterRegistry reg;
    EXPECT_FALSE(reg.Detach("Unknown"));
}

TEST(AdapterRegistry, IsAttachedQuery) {
    AdapterRegistry reg;
    reg.Register(std::make_shared<StubAdapter>("UIA"));
    EXPECT_FALSE(reg.IsAttached("UIA"));
    reg.Attach("UIA");
    EXPECT_TRUE(reg.IsAttached("UIA"));
}

TEST(AdapterRegistry, UnregisterDetachesFirst) {
    AdapterRegistry reg;
    auto stub = std::make_shared<StubAdapter>("UIA");
    reg.Register(stub);
    reg.Attach("UIA");
    ASSERT_TRUE(stub->IsAttached());
    reg.Unregister("UIA");
    EXPECT_FALSE(stub->IsAttached());
    EXPECT_EQ(stub->DetachCount(), 1);
}

// ── Event routing tests ───────────────────────────────────────────────────────

TEST(AdapterRegistry, EventsRoutedToPipeline) {
    AdapterRegistry reg;
    auto stub = std::make_shared<StubAdapter>("UIA");
    reg.Register(stub);

    int received = 0;
    reg.SetPipelineCallback([&](const AdapterEvent&) { ++received; });
    reg.Attach("UIA");

    stub->FireEvent(AdapterEventKind::FocusChanged, "Notepad");
    EXPECT_EQ(received, 1);
}

TEST(AdapterRegistry, EventPayloadPreserved) {
    AdapterRegistry reg;
    auto stub = std::make_shared<StubAdapter>("Browser");
    reg.Register(stub);

    AdapterEvent last;
    reg.SetPipelineCallback([&](const AdapterEvent& ev) { last = ev; });
    reg.Attach("Browser");

    stub->FireEvent(AdapterEventKind::BrowserFocus, "Google");
    EXPECT_EQ(last.sourceName,  "Browser");
    EXPECT_EQ(last.elementName, "Google");
    EXPECT_EQ(last.kind,        AdapterEventKind::BrowserFocus);
}

TEST(AdapterRegistry, MultipleAdaptersRouteToSamePipeline) {
    AdapterRegistry reg;
    auto s1 = std::make_shared<StubAdapter>("UIA");
    auto s2 = std::make_shared<StubAdapter>("Browser");
    reg.Register(s1);
    reg.Register(s2);

    int count = 0;
    reg.SetPipelineCallback([&](const AdapterEvent&) { ++count; });
    reg.AttachAll();

    s1->FireEvent(AdapterEventKind::FocusChanged);
    s2->FireEvent(AdapterEventKind::BrowserPageLoad);
    EXPECT_EQ(count, 2);
}

// ── Enable / disable tests ────────────────────────────────────────────────────

TEST(AdapterRegistry, DisabledAdapterEventsNotRouted) {
    AdapterRegistry reg;
    auto stub = std::make_shared<StubAdapter>("UIA");
    reg.Register(stub);
    reg.Attach("UIA");

    int received = 0;
    reg.SetPipelineCallback([&](const AdapterEvent&) { ++received; });

    reg.SetEnabled("UIA", false);
    stub->FireEvent(AdapterEventKind::FocusChanged);
    EXPECT_EQ(received, 0);
}

TEST(AdapterRegistry, ReEnableRoutesEvents) {
    AdapterRegistry reg;
    auto stub = std::make_shared<StubAdapter>("UIA");
    reg.Register(stub);
    reg.Attach("UIA");

    int received = 0;
    reg.SetPipelineCallback([&](const AdapterEvent&) { ++received; });

    reg.SetEnabled("UIA", false);
    stub->FireEvent(AdapterEventKind::FocusChanged);
    EXPECT_EQ(received, 0);

    reg.SetEnabled("UIA", true);
    stub->FireEvent(AdapterEventKind::FocusChanged);
    EXPECT_EQ(received, 1);
}

TEST(AdapterRegistry, IsEnabledDefaultTrue) {
    AdapterRegistry reg;
    reg.Register(std::make_shared<StubAdapter>("UIA"));
    EXPECT_TRUE(reg.IsEnabled("UIA"));
}

TEST(AdapterRegistry, IsEnabledUnknownFalse) {
    AdapterRegistry reg;
    EXPECT_FALSE(reg.IsEnabled("NoSuchAdapter"));
}

TEST(AdapterRegistry, OnlyDisabledAdapterFiltered) {
    AdapterRegistry reg;
    auto s1 = std::make_shared<StubAdapter>("UIA");
    auto s2 = std::make_shared<StubAdapter>("Browser");
    reg.Register(s1);
    reg.Register(s2);
    reg.AttachAll();

    int received = 0;
    reg.SetPipelineCallback([&](const AdapterEvent&) { ++received; });

    reg.SetEnabled("UIA", false);
    s1->FireEvent(AdapterEventKind::FocusChanged);   // suppressed
    s2->FireEvent(AdapterEventKind::BrowserPageLoad); // passes
    EXPECT_EQ(received, 1);
}

// ── No-pipeline-callback: events silently dropped (no crash) ──────────────────

TEST(AdapterRegistry, NoPipelineCallbackNocrash) {
    AdapterRegistry reg;
    auto stub = std::make_shared<StubAdapter>("UIA");
    reg.Register(stub);
    reg.Attach("UIA");
    EXPECT_NO_FATAL_FAILURE(stub->FireEvent(AdapterEventKind::FocusChanged));
}
