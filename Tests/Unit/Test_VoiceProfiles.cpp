// Test_VoiceProfiles.cpp — ACCESSOS-040 unit tests
// Tests for VoiceProfileManager: CRUD, active profile, EnsureDefault, list ops.

#include <gtest/gtest.h>
#include "../../src/Core/Speech/VoiceProfile.h"
#include "../../src/Core/Settings/SettingsManager.h"
#include "../../src/Core/Settings/ISettingsStore.h"

#include <optional>
#include <string>
#include <unordered_map>

using namespace AccessOS;
using namespace AccessOS::Speech;

// ── In-memory store for tests ─────────────────────────────────────────────────

class MemStore : public ISettingsStore {
public:
    bool Open(const std::string&) override { return true; }
    void Close() override {}
    bool IsOpen() const noexcept override { return true; }

    bool Set(const std::string& key, const std::string& val) override {
        m_data[key] = val; return true;
    }
    std::optional<std::string> Get(const std::string& key) const override {
        auto it = m_data.find(key);
        if (it == m_data.end()) return std::nullopt;
        return it->second;
    }
    std::string GetOr(const std::string& key, const std::string& def) const override {
        auto v = Get(key);
        return v.value_or(def);
    }
    bool Remove(const std::string& key) override {
        return m_data.erase(key) > 0;
    }
    bool Has(const std::string& key) const override {
        return m_data.count(key) > 0;
    }
    std::vector<std::string> Keys(const std::string& prefix) const override {
        std::vector<std::string> out;
        for (auto& [k, v] : m_data)
            if (k.substr(0, prefix.size()) == prefix) out.push_back(k);
        return out;
    }
    void Clear(const std::string& prefix) override {
        if (prefix.empty()) { m_data.clear(); return; }
        for (auto it = m_data.begin(); it != m_data.end(); ) {
            if (it->first.substr(0, prefix.size()) == prefix) it = m_data.erase(it);
            else ++it;
        }
    }
private:
    std::unordered_map<std::string, std::string> m_data;
};

// ── Fixture ───────────────────────────────────────────────────────────────────

class VoiceProfileTest : public ::testing::Test {
protected:
    void SetUp() override {
        store   = std::make_shared<MemStore>();
        settings= std::make_unique<SettingsManager>(store);
        mgr     = std::make_unique<VoiceProfileManager>(*settings);
    }

    std::shared_ptr<MemStore>           store;
    std::unique_ptr<SettingsManager>    settings;
    std::unique_ptr<VoiceProfileManager> mgr;
};

// ── Save / Load ───────────────────────────────────────────────────────────────

TEST_F(VoiceProfileTest, SaveAndLoadRoundtrip) {
    VoiceProfile p;
    p.name    = "Test";
    p.voiceId = "Microsoft David Desktop";
    p.rate    = 3;
    p.volume  = 80;
    mgr->Save(p);

    auto loaded = mgr->Load("Test");
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->name,    "Test");
    EXPECT_EQ(loaded->voiceId, "Microsoft David Desktop");
    EXPECT_EQ(loaded->rate,    3);
    EXPECT_EQ(loaded->volume,  80);
}

TEST_F(VoiceProfileTest, LoadNonExistentReturnsNullopt) {
    auto result = mgr->Load("DoesNotExist");
    EXPECT_FALSE(result.has_value());
}

TEST_F(VoiceProfileTest, SaveEmptyNameIsNoOp) {
    VoiceProfile p; p.name = "";
    mgr->Save(p);
    EXPECT_TRUE(mgr->ListNames().empty());
}

TEST_F(VoiceProfileTest, SaveOverwritesExisting) {
    VoiceProfile p; p.name = "A"; p.rate = 0; p.volume = 100;
    mgr->Save(p);
    p.rate = 5;
    mgr->Save(p);

    auto loaded = mgr->Load("A");
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->rate, 5);
}

TEST_F(VoiceProfileTest, SaveDefaultValues) {
    VoiceProfile p; p.name = "Default";
    mgr->Save(p);
    auto loaded = mgr->Load("Default");
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->rate,   0);
    EXPECT_EQ(loaded->volume, 100);
}

// ── Delete ────────────────────────────────────────────────────────────────────

TEST_F(VoiceProfileTest, DeleteExistingReturnsTrue) {
    VoiceProfile p; p.name = "ToDelete";
    mgr->Save(p);
    EXPECT_TRUE(mgr->Delete("ToDelete"));
    EXPECT_FALSE(mgr->Load("ToDelete").has_value());
}

TEST_F(VoiceProfileTest, DeleteNonExistentReturnsFalse) {
    EXPECT_FALSE(mgr->Delete("Ghost"));
}

TEST_F(VoiceProfileTest, DeleteEmptyNameReturnsFalse) {
    EXPECT_FALSE(mgr->Delete(""));
}

TEST_F(VoiceProfileTest, DeleteRemovesFromList) {
    VoiceProfile p; p.name = "X";
    mgr->Save(p);
    mgr->Delete("X");
    auto names = mgr->ListNames();
    EXPECT_TRUE(std::find(names.begin(), names.end(), "X") == names.end());
}

TEST_F(VoiceProfileTest, DeleteActiveProfileClearsActive) {
    VoiceProfile p; p.name = "Active";
    mgr->Save(p);
    mgr->SetActive("Active");
    mgr->Delete("Active");
    EXPECT_EQ(mgr->GetActive(), "");
}

// ── List ──────────────────────────────────────────────────────────────────────

TEST_F(VoiceProfileTest, ListNamesEmptyOnFresh) {
    EXPECT_TRUE(mgr->ListNames().empty());
}

TEST_F(VoiceProfileTest, ListNamesReturnsAllSaved) {
    for (auto name : {"Alpha", "Beta", "Gamma"}) {
        VoiceProfile p; p.name = name;
        mgr->Save(p);
    }
    auto names = mgr->ListNames();
    EXPECT_EQ(names.size(), 3u);
}

TEST_F(VoiceProfileTest, SaveSameNameTwiceNotDuplicated) {
    VoiceProfile p; p.name = "Dup";
    mgr->Save(p);
    mgr->Save(p);
    EXPECT_EQ(mgr->ListNames().size(), 1u);
}

// ── Active profile ────────────────────────────────────────────────────────────

TEST_F(VoiceProfileTest, GetActiveEmptyByDefault) {
    EXPECT_EQ(mgr->GetActive(), "");
}

TEST_F(VoiceProfileTest, SetAndGetActive) {
    mgr->SetActive("MyProfile");
    EXPECT_EQ(mgr->GetActive(), "MyProfile");
}

TEST_F(VoiceProfileTest, LoadActiveNulloptWhenNoneSet) {
    EXPECT_FALSE(mgr->LoadActive().has_value());
}

TEST_F(VoiceProfileTest, LoadActiveReturnsSavedProfile) {
    VoiceProfile p; p.name = "Live"; p.voiceId = "Zira"; p.rate = -2; p.volume = 90;
    mgr->Save(p);
    mgr->SetActive("Live");

    auto active = mgr->LoadActive();
    ASSERT_TRUE(active.has_value());
    EXPECT_EQ(active->name,    "Live");
    EXPECT_EQ(active->voiceId, "Zira");
    EXPECT_EQ(active->rate,    -2);
    EXPECT_EQ(active->volume,  90);
}

TEST_F(VoiceProfileTest, LoadActiveNulloptWhenActiveNotFound) {
    mgr->SetActive("Missing");
    EXPECT_FALSE(mgr->LoadActive().has_value());
}

// ── EnsureDefault ─────────────────────────────────────────────────────────────

TEST_F(VoiceProfileTest, EnsureDefaultCreatesProfile) {
    auto def = mgr->EnsureDefault();
    EXPECT_EQ(def.name, "Default");
    EXPECT_TRUE(mgr->Load("Default").has_value());
}

TEST_F(VoiceProfileTest, EnsureDefaultDoesNotOverwriteExisting) {
    VoiceProfile p; p.name = "Default"; p.voiceId = "CustomVoice"; p.rate = 7;
    mgr->Save(p);

    auto def = mgr->EnsureDefault();
    EXPECT_EQ(def.voiceId, "CustomVoice");
    EXPECT_EQ(def.rate,    7);
}

TEST_F(VoiceProfileTest, EnsureDefaultAppearsInList) {
    mgr->EnsureDefault();
    auto names = mgr->ListNames();
    EXPECT_NE(std::find(names.begin(), names.end(), "Default"), names.end());
}

// ── Multiple profiles ─────────────────────────────────────────────────────────

TEST_F(VoiceProfileTest, MultipleProfilesIndependent) {
    VoiceProfile slow; slow.name = "Slow"; slow.rate = -5; slow.volume = 70;
    VoiceProfile fast; fast.name = "Fast"; fast.rate =  8; fast.volume = 100;
    mgr->Save(slow);
    mgr->Save(fast);

    auto s = mgr->Load("Slow");
    auto f = mgr->Load("Fast");
    ASSERT_TRUE(s.has_value()); ASSERT_TRUE(f.has_value());
    EXPECT_EQ(s->rate, -5);
    EXPECT_EQ(f->rate,  8);
}

TEST_F(VoiceProfileTest, ProfileEquality) {
    VoiceProfile a; a.name = "P"; a.voiceId = "V"; a.rate = 1; a.volume = 50;
    VoiceProfile b = a;
    EXPECT_EQ(a, b);
    b.rate = 2;
    EXPECT_NE(a, b);
}
