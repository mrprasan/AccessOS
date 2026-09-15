// Tests/Unit/Test_SettingsManager.cpp
//
// Unit tests for ISettingsStore, SqliteSettingsStore, and SettingsManager.
// Uses in-memory SQLite (:memory:) — no file system access required.

#include <gtest/gtest.h>
#include "Settings/SqliteSettingsStore.h"
#include "Settings/SettingsManager.h"

#include <memory>
#include <string>

using namespace AccessOS;

// ─── Helper: open in-memory store ────────────────────────────────────────────

static std::shared_ptr<SqliteSettingsStore> MakeStore() {
    auto store = std::make_shared<SqliteSettingsStore>();
    EXPECT_TRUE(store->Open(":memory:"));
    return store;
}

// ─── SqliteSettingsStore — open / close ──────────────────────────────────────

TEST(SqliteSettingsStoreTest, OpenInMemorySucceeds) {
    SqliteSettingsStore store;
    EXPECT_TRUE(store.Open(":memory:"));
    EXPECT_TRUE(store.IsOpen());
    store.Close();
    EXPECT_FALSE(store.IsOpen());
}

TEST(SqliteSettingsStoreTest, DoubleOpenIsIdempotent) {
    SqliteSettingsStore store;
    EXPECT_TRUE(store.Open(":memory:"));
    EXPECT_TRUE(store.Open(":memory:"));  // Second open is a no-op.
    EXPECT_TRUE(store.IsOpen());
}

// ─── SqliteSettingsStore — Set / Get ─────────────────────────────────────────

TEST(SqliteSettingsStoreTest, SetAndGetRoundTrip) {
    auto store = MakeStore();
    EXPECT_TRUE(store->Set("speech.rate", "5"));
    auto v = store->Get("speech.rate");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, "5");
}

TEST(SqliteSettingsStoreTest, GetMissingKeyReturnsNullopt) {
    auto store = MakeStore();
    EXPECT_FALSE(store->Get("does.not.exist").has_value());
}

TEST(SqliteSettingsStoreTest, GetOrReturnsFallback) {
    auto store = MakeStore();
    EXPECT_EQ(store->GetOr("missing.key", "default"), "default");
}

TEST(SqliteSettingsStoreTest, SetOverwritesExistingValue) {
    auto store = MakeStore();
    store->Set("k", "first");
    store->Set("k", "second");
    EXPECT_EQ(*store->Get("k"), "second");
}

// ─── SqliteSettingsStore — Has ────────────────────────────────────────────────

TEST(SqliteSettingsStoreTest, HasReturnsTrueForExistingKey) {
    auto store = MakeStore();
    store->Set("x", "1");
    EXPECT_TRUE(store->Has("x"));
}

TEST(SqliteSettingsStoreTest, HasReturnsFalseForMissingKey) {
    auto store = MakeStore();
    EXPECT_FALSE(store->Has("ghost"));
}

// ─── SqliteSettingsStore — Remove ─────────────────────────────────────────────

TEST(SqliteSettingsStoreTest, RemoveDeletesKey) {
    auto store = MakeStore();
    store->Set("del.me", "value");
    EXPECT_TRUE(store->Remove("del.me"));
    EXPECT_FALSE(store->Has("del.me"));
}

TEST(SqliteSettingsStoreTest, RemoveMissingKeyReturnsFalse) {
    auto store = MakeStore();
    EXPECT_FALSE(store->Remove("ghost.key"));
}

// ─── SqliteSettingsStore — Keys ───────────────────────────────────────────────

TEST(SqliteSettingsStoreTest, KeysReturnsAllWhenNoPrefixGiven) {
    auto store = MakeStore();
    store->Set("a.1", "v");
    store->Set("b.2", "v");
    store->Set("c.3", "v");
    EXPECT_EQ(store->Keys().size(), 3u);
}

TEST(SqliteSettingsStoreTest, KeysWithPrefixFiltersCorrectly) {
    auto store = MakeStore();
    store->Set("speech.rate",   "0");
    store->Set("speech.volume", "100");
    store->Set("ui.theme",      "dark");

    const auto keys = store->Keys("speech.");
    EXPECT_EQ(keys.size(), 2u);
    for (const auto& k : keys) {
        EXPECT_EQ(k.substr(0, 7), "speech.");
    }
}

TEST(SqliteSettingsStoreTest, KeysEmptyWhenNoneMatch) {
    auto store = MakeStore();
    store->Set("ui.theme", "light");
    EXPECT_TRUE(store->Keys("speech.").empty());
}

// ─── SqliteSettingsStore — Clear ──────────────────────────────────────────────

TEST(SqliteSettingsStoreTest, ClearAllRemovesEverything) {
    auto store = MakeStore();
    store->Set("a", "1");
    store->Set("b", "2");
    store->Clear();
    EXPECT_EQ(store->Keys().size(), 0u);
}

TEST(SqliteSettingsStoreTest, ClearWithPrefixRemovesOnlyMatching) {
    auto store = MakeStore();
    store->Set("speech.rate",   "0");
    store->Set("speech.volume", "100");
    store->Set("ui.theme",      "dark");
    store->Clear("speech.");
    EXPECT_FALSE(store->Has("speech.rate"));
    EXPECT_FALSE(store->Has("speech.volume"));
    EXPECT_TRUE(store->Has("ui.theme"));   // Unaffected.
}

// ─── SettingsManager — speech ─────────────────────────────────────────────────

TEST(SettingsManagerTest, DefaultSpeechRate) {
    SettingsManager mgr(MakeStore());
    EXPECT_EQ(mgr.SpeechRate(), 0);
}

TEST(SettingsManagerTest, SetAndGetSpeechRate) {
    SettingsManager mgr(MakeStore());
    mgr.SetSpeechRate(5);
    EXPECT_EQ(mgr.SpeechRate(), 5);
}

TEST(SettingsManagerTest, DefaultSpeechVolume) {
    SettingsManager mgr(MakeStore());
    EXPECT_EQ(mgr.SpeechVolume(), 100);
}

TEST(SettingsManagerTest, SetAndGetSpeechVolume) {
    SettingsManager mgr(MakeStore());
    mgr.SetSpeechVolume(75);
    EXPECT_EQ(mgr.SpeechVolume(), 75);
}

TEST(SettingsManagerTest, DefaultVoiceIdIsEmpty) {
    SettingsManager mgr(MakeStore());
    EXPECT_EQ(mgr.VoiceId(), "");
}

TEST(SettingsManagerTest, SetAndGetVoiceId) {
    SettingsManager mgr(MakeStore());
    mgr.SetVoiceId("Microsoft David Desktop");
    EXPECT_EQ(mgr.VoiceId(), "Microsoft David Desktop");
}

TEST(SettingsManagerTest, DefaultVerbosityIsOne) {
    SettingsManager mgr(MakeStore());
    EXPECT_EQ(mgr.Verbosity(), 1);
}

TEST(SettingsManagerTest, SetAndGetVerbosity) {
    SettingsManager mgr(MakeStore());
    mgr.SetVerbosity(2);
    EXPECT_EQ(mgr.Verbosity(), 2);
}

TEST(SettingsManagerTest, AnnouncePositionDefaultFalse) {
    SettingsManager mgr(MakeStore());
    EXPECT_FALSE(mgr.AnnouncePosition());
}

TEST(SettingsManagerTest, SetAnnouncePositionTrue) {
    SettingsManager mgr(MakeStore());
    mgr.SetAnnouncePosition(true);
    EXPECT_TRUE(mgr.AnnouncePosition());
}

// ─── SettingsManager — shortcuts ─────────────────────────────────────────────

TEST(SettingsManagerTest, ShortcutDefaultIsZeroZero) {
    SettingsManager mgr(MakeStore());
    auto [vk, mod] = mgr.GetShortcut("nav.next");
    EXPECT_EQ(vk,  0u);
    EXPECT_EQ(mod, 0u);
}

TEST(SettingsManagerTest, SetAndGetShortcut) {
    SettingsManager mgr(MakeStore());
    mgr.SetShortcut("nav.next", 0x48 /*H*/, 0x02 /*Ctrl*/);
    auto [vk, mod] = mgr.GetShortcut("nav.next");
    EXPECT_EQ(vk,  0x48u);
    EXPECT_EQ(mod, 0x02u);
}

TEST(SettingsManagerTest, RemoveShortcut) {
    SettingsManager mgr(MakeStore());
    mgr.SetShortcut("speech.stop", 0x53 /*S*/, 0x02);
    mgr.RemoveShortcut("speech.stop");
    auto [vk, mod] = mgr.GetShortcut("speech.stop");
    EXPECT_EQ(vk,  0u);
    EXPECT_EQ(mod, 0u);
}

// ─── SettingsManager — UI ────────────────────────────────────────────────────

TEST(SettingsManagerTest, DefaultThemeIsSystem) {
    SettingsManager mgr(MakeStore());
    EXPECT_EQ(mgr.Theme(), "system");
}

TEST(SettingsManagerTest, SetThemeDark) {
    SettingsManager mgr(MakeStore());
    mgr.SetTheme("dark");
    EXPECT_EQ(mgr.Theme(), "dark");
}

// ─── SettingsManager — privacy ───────────────────────────────────────────────

TEST(SettingsManagerTest, SuppressPasswordsDefaultTrue) {
    SettingsManager mgr(MakeStore());
    EXPECT_TRUE(mgr.SuppressPasswords());
}

TEST(SettingsManagerTest, SetSuppressPasswordsFalse) {
    SettingsManager mgr(MakeStore());
    mgr.SetSuppressPasswords(false);
    EXPECT_FALSE(mgr.SuppressPasswords());
}

// ─── SettingsManager — persistence across open/close ─────────────────────────

TEST(SettingsManagerTest, ValuesPersistedToFileAndReloaded) {
    const std::string path = "test_settings_persistence.db";

    {
        auto store = std::make_shared<SqliteSettingsStore>();
        ASSERT_TRUE(store->Open(path));
        SettingsManager mgr(store);
        mgr.SetSpeechRate(7);
        mgr.SetVoiceId("TestVoice");
        mgr.SetTheme("dark");
        store->Close();
    }

    {
        auto store = std::make_shared<SqliteSettingsStore>();
        ASSERT_TRUE(store->Open(path));
        SettingsManager mgr(store);
        EXPECT_EQ(mgr.SpeechRate(), 7);
        EXPECT_EQ(mgr.VoiceId(), "TestVoice");
        EXPECT_EQ(mgr.Theme(), "dark");
        store->Close();
    }

    // Clean up.
    std::remove(path.c_str());
}
