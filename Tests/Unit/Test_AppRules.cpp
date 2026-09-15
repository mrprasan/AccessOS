// Test_AppRules.cpp — ACCESSOS-042 unit tests
// Tests for AppRuleManager: CRUD, matching, per-field helpers, encode/decode.

#include <gtest/gtest.h>
#include "../../src/Core/Rules/AppRule.h"
#include "../../src/Core/Settings/SettingsManager.h"
#include "../../src/Core/Settings/ISettingsStore.h"

#include <optional>
#include <unordered_map>
#include <string>

using namespace AccessOS;
using namespace AccessOS::Rules;

// ── Reuse in-memory store ─────────────────────────────────────────────────────

class MemStore2 : public ISettingsStore {
public:
    bool Open(const std::string&) override { return true; }
    void Close() override {}
    bool IsOpen() const noexcept override { return true; }
    bool Set(const std::string& k, const std::string& v) override {
        m[k] = v; return true;
    }
    std::optional<std::string> Get(const std::string& k) const override {
        auto it = m.find(k);
        if (it == m.end()) return std::nullopt;
        return it->second;
    }
    std::string GetOr(const std::string& k, const std::string& d) const override {
        return Get(k).value_or(d);
    }
    bool Remove(const std::string& k) override { return m.erase(k) > 0; }
    bool Has(const std::string& k) const override { return m.count(k) > 0; }
    std::vector<std::string> Keys(const std::string& p) const override {
        std::vector<std::string> out;
        for (auto& [k, v] : m)
            if (k.substr(0, p.size()) == p) out.push_back(k);
        return out;
    }
    void Clear(const std::string& p) override {
        if (p.empty()) { m.clear(); return; }
        for (auto it = m.begin(); it != m.end(); )
            if (it->first.substr(0, p.size()) == p) it = m.erase(it);
            else ++it;
    }
private:
    std::unordered_map<std::string, std::string> m;
};

class AppRuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        store    = std::make_shared<MemStore2>();
        settings = std::make_unique<SettingsManager>(store);
        mgr      = std::make_unique<AppRuleManager>(*settings);
    }
    std::shared_ptr<MemStore2>         store;
    std::unique_ptr<SettingsManager>   settings;
    std::unique_ptr<AppRuleManager>    mgr;
};

// ── Save / Get ────────────────────────────────────────────────────────────────

TEST_F(AppRuleTest, SaveAndGetRoundtrip) {
    AppRule r;
    r.exeName       = "notepad.exe";
    r.verbosity     = 2;
    r.browseModeAuto= true;
    r.earconsEnabled= false;
    mgr->Save(r);

    auto loaded = mgr->Get("notepad.exe");
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->exeName, "notepad.exe");
    ASSERT_TRUE(loaded->verbosity.has_value());
    EXPECT_EQ(*loaded->verbosity, 2);
    ASSERT_TRUE(loaded->browseModeAuto.has_value());
    EXPECT_TRUE(*loaded->browseModeAuto);
    ASSERT_TRUE(loaded->earconsEnabled.has_value());
    EXPECT_FALSE(*loaded->earconsEnabled);
}

TEST_F(AppRuleTest, GetNonExistentReturnsNullopt) {
    EXPECT_FALSE(mgr->Get("ghost.exe").has_value());
}

TEST_F(AppRuleTest, GetEmptyExeReturnsNullopt) {
    EXPECT_FALSE(mgr->Get("").has_value());
}

TEST_F(AppRuleTest, SaveEmptyExeIsNoOp) {
    AppRule r; r.exeName = "";
    mgr->Save(r);
    EXPECT_TRUE(mgr->ListExeNames().empty());
}

TEST_F(AppRuleTest, SaveNormalisesToLowercase) {
    AppRule r; r.exeName = "Notepad.EXE"; r.verbosity = 1;
    mgr->Save(r);
    EXPECT_TRUE(mgr->Get("notepad.exe").has_value());
}

TEST_F(AppRuleTest, SaveOverwritesExisting) {
    AppRule r; r.exeName = "a.exe"; r.verbosity = 0;
    mgr->Save(r);
    r.verbosity = 3;
    mgr->Save(r);
    EXPECT_EQ(*mgr->Get("a.exe")->verbosity, 3);
}

TEST_F(AppRuleTest, NulloptFieldsStoredAsAbsent) {
    AppRule r; r.exeName = "b.exe"; // no optional fields set
    mgr->Save(r);
    auto loaded = mgr->Get("b.exe");
    ASSERT_TRUE(loaded.has_value());
    EXPECT_FALSE(loaded->verbosity.has_value());
    EXPECT_FALSE(loaded->browseModeAuto.has_value());
    EXPECT_FALSE(loaded->earconsEnabled.has_value());
}

// ── Delete ────────────────────────────────────────────────────────────────────

TEST_F(AppRuleTest, DeleteExistingReturnsTrue) {
    AppRule r; r.exeName = "del.exe";
    mgr->Save(r);
    EXPECT_TRUE(mgr->Delete("del.exe"));
    EXPECT_FALSE(mgr->Get("del.exe").has_value());
}

TEST_F(AppRuleTest, DeleteNonExistentReturnsFalse) {
    EXPECT_FALSE(mgr->Delete("nosuch.exe"));
}

TEST_F(AppRuleTest, DeleteRemovesFromList) {
    AppRule r; r.exeName = "c.exe";
    mgr->Save(r);
    mgr->Delete("c.exe");
    EXPECT_TRUE(mgr->ListExeNames().empty());
}

// ── List ──────────────────────────────────────────────────────────────────────

TEST_F(AppRuleTest, ListExeNamesEmptyOnFresh) {
    EXPECT_TRUE(mgr->ListExeNames().empty());
}

TEST_F(AppRuleTest, ListExeNamesReturnsAll) {
    for (auto e : {"a.exe", "b.exe", "c.exe"}) {
        AppRule r; r.exeName = e;
        mgr->Save(r);
    }
    EXPECT_EQ(mgr->ListExeNames().size(), 3u);
}

TEST_F(AppRuleTest, SaveSameExeTwiceNotDuplicated) {
    AppRule r; r.exeName = "dup.exe";
    mgr->Save(r);
    mgr->Save(r);
    EXPECT_EQ(mgr->ListExeNames().size(), 1u);
}

// ── Role templates ────────────────────────────────────────────────────────────

TEST_F(AppRuleTest, SetRoleTemplateStored) {
    mgr->SetRoleTemplate("app.exe", "button", "{name}, tap");
    auto rule = mgr->Get("app.exe");
    ASSERT_TRUE(rule.has_value());
    ASSERT_EQ(rule->roleTemplates.count("button"), 1u);
    EXPECT_EQ(rule->roleTemplates.at("button"), "{name}, tap");
}

TEST_F(AppRuleTest, RemoveRoleTemplate) {
    mgr->SetRoleTemplate("app.exe", "link", "link {name}");
    mgr->RemoveRoleTemplate("app.exe", "link");
    auto rule = mgr->Get("app.exe");
    ASSERT_TRUE(rule.has_value());
    EXPECT_EQ(rule->roleTemplates.count("link"), 0u);
}

TEST_F(AppRuleTest, MultipleRoleTemplates) {
    mgr->SetRoleTemplate("app.exe", "button", "B:{name}");
    mgr->SetRoleTemplate("app.exe", "link",   "L:{name}");
    auto rule = mgr->Get("app.exe");
    EXPECT_EQ(rule->roleTemplates.size(), 2u);
}

// ── Per-field helpers ─────────────────────────────────────────────────────────

TEST_F(AppRuleTest, SetVerbosityCreatesRule) {
    mgr->SetVerbosity("app2.exe", 1);
    EXPECT_EQ(*mgr->Get("app2.exe")->verbosity, 1);
}

TEST_F(AppRuleTest, SetBrowseModeAutoTrue) {
    mgr->SetBrowseModeAuto("browser.exe", true);
    EXPECT_TRUE(*mgr->Get("browser.exe")->browseModeAuto);
}

TEST_F(AppRuleTest, SetEarconsDisabled) {
    mgr->SetEarconsEnabled("game.exe", false);
    EXPECT_FALSE(*mgr->Get("game.exe")->earconsEnabled);
}

// ── Match (case-insensitive) ──────────────────────────────────────────────────

TEST_F(AppRuleTest, MatchCaseInsensitive) {
    AppRule r; r.exeName = "calc.exe"; r.verbosity = 0;
    mgr->Save(r);
    EXPECT_TRUE(mgr->Match("CALC.EXE").has_value());
    EXPECT_TRUE(mgr->Match("Calc.Exe").has_value());
}

TEST_F(AppRuleTest, MatchEmptyReturnsNullopt) {
    EXPECT_FALSE(mgr->Match("").has_value());
}

TEST_F(AppRuleTest, MatchNoRuleReturnsNullopt) {
    EXPECT_FALSE(mgr->Match("unknown.exe").has_value());
}
