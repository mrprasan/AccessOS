// Test_OcrSubsystem.cpp — ACCESSOS-022 OCR Subsystem tests
// Covers: OcrResult model, OcrManager lifecycle, filter logic, stub engine

#include <gtest/gtest.h>
#include "../../src/Core/OCR/OcrResult.h"
#include "../../src/Core/OCR/IOcrEngine.h"
#include "../../src/Core/OCR/OcrManager.h"

using namespace AccessOS::OCR;

// ── Stub OCR engine ───────────────────────────────────────────────────────────

class StubOcrEngine : public IOcrEngine {
public:
    explicit StubOcrEngine(bool available = true) : m_available(available) {}

    bool IsAvailable() const override { return m_available; }

    // Returns a pre-programmed result
    OcrResult RecognizeFromBitmap(HBITMAP, const OcrRect& region,
                                  const OcrOptions& opts) override {
        return MakeResult(region, opts.languageTag);
    }

    OcrResult RecognizeFromFile(const std::string& /*path*/,
                                const OcrOptions& opts) override {
        return MakeResult({}, opts.languageTag);
    }

    OcrResult RecognizeScreenRegion(const OcrRect& screenRect,
                                    const OcrOptions& opts) override {
        ++m_callCount;
        return MakeResult(screenRect, opts.languageTag);
    }

    int CallCount() const { return m_callCount; }

    // Configure what words to return
    void SetWords(const std::vector<std::pair<std::string,float>>& words) {
        m_words = words;
    }

private:
    bool m_available;
    int  m_callCount = 0;
    std::vector<std::pair<std::string,float>> m_words = {
        {"Hello", 0.95f}, {"World", 0.88f}
    };

    OcrResult MakeResult(const OcrRect& region, const std::string& lang) const {
        OcrResult r;
        r.succeeded   = m_available;
        r.sourceRect  = region;
        r.languageTag = lang;
        if (!m_available) return r;

        OcrLine line;
        int x = 0;
        for (auto& [text, conf] : m_words) {
            OcrWord w;
            w.text       = text;
            w.confidence = conf;
            w.bounds     = { x, 0, static_cast<int>(text.size()) * 8, 16 };
            x           += w.bounds.width + 4;
            line.words.push_back(std::move(w));
        }
        r.lines.push_back(std::move(line));
        return r;
    }
};

// ── OcrRect tests ─────────────────────────────────────────────────────────────

TEST(OcrRect, IsEmptyTrueOnDefault) {
    OcrRect r{};
    EXPECT_TRUE(r.IsEmpty());
}

TEST(OcrRect, IsEmptyFalseOnValidRect) {
    OcrRect r{ 0, 0, 100, 50 };
    EXPECT_FALSE(r.IsEmpty());
}

TEST(OcrRect, ContainsPoint) {
    OcrRect r{ 10, 10, 80, 40 };
    EXPECT_TRUE(r.Contains(50, 30));
    EXPECT_FALSE(r.Contains(5, 5));
    EXPECT_FALSE(r.Contains(90, 50)); // boundary exclusive
}

TEST(OcrRect, ContainsTopLeftCorner) {
    OcrRect r{ 10, 10, 80, 40 };
    EXPECT_TRUE(r.Contains(10, 10));
}

// ── OcrWord / OcrLine tests ───────────────────────────────────────────────────

TEST(OcrLine, TextConcatenatesWords) {
    OcrLine line;
    OcrWord w1; w1.text = "Hello"; w1.confidence = 1.0f;
    OcrWord w2; w2.text = "World"; w2.confidence = 1.0f;
    line.words = { w1, w2 };
    EXPECT_EQ(line.Text(), "Hello World");
}

TEST(OcrLine, TextEmptyLine) {
    OcrLine line;
    EXPECT_EQ(line.Text(), "");
}

TEST(OcrLine, BoundsUnionOfWords) {
    OcrLine line;
    OcrWord w1; w1.bounds = { 0, 0, 40, 16 };
    OcrWord w2; w2.bounds = { 50, 2, 40, 14 };
    line.words = { w1, w2 };
    OcrRect b = line.Bounds();
    EXPECT_EQ(b.x,      0);
    EXPECT_EQ(b.y,      0);
    EXPECT_EQ(b.width,  90); // 0 to 90
    EXPECT_EQ(b.height, 16);
}

TEST(OcrLine, BoundsEmptyLineReturnsDefault) {
    OcrLine line;
    OcrRect b = line.Bounds();
    EXPECT_TRUE(b.IsEmpty());
}

// ── OcrResult tests ───────────────────────────────────────────────────────────

TEST(OcrResult, FullTextMultiLine) {
    OcrResult r;
    OcrLine l1; OcrWord w1; w1.text="Hello"; l1.words={w1};
    OcrLine l2; OcrWord w2; w2.text="World"; l2.words={w2};
    r.lines = { l1, l2 };
    EXPECT_EQ(r.FullText(), "Hello\nWorld");
}

TEST(OcrResult, FullTextEmpty) {
    OcrResult r;
    EXPECT_EQ(r.FullText(), "");
}

TEST(OcrResult, WordCountAcrossLines) {
    OcrResult r;
    OcrLine l1; OcrWord w1; w1.text="a"; OcrWord w2; w2.text="b";
    l1.words={w1,w2};
    OcrLine l2; OcrWord w3; w3.text="c"; l2.words={w3};
    r.lines = { l1, l2 };
    EXPECT_EQ(r.WordCount(), 3u);
}

TEST(OcrResult, IsEmptyTrueWhenNoLines) {
    OcrResult r;
    EXPECT_TRUE(r.IsEmpty());
}

TEST(OcrResult, IsEmptyFalseWhenHasLines) {
    OcrResult r;
    OcrLine l; OcrWord w; w.text="x"; l.words={w};
    r.lines = {l};
    EXPECT_FALSE(r.IsEmpty());
}

// ── OcrManager lifecycle tests ────────────────────────────────────────────────

TEST(OcrManager, DefaultConstructorNotAvailable) {
    OcrManager mgr;
    EXPECT_FALSE(mgr.IsAvailable());
}

TEST(OcrManager, SetEngineAvailable) {
    OcrManager mgr;
    mgr.SetEngine(std::make_shared<StubOcrEngine>(true));
    EXPECT_TRUE(mgr.IsAvailable());
}

TEST(OcrManager, SetEngineUnavailable) {
    OcrManager mgr;
    mgr.SetEngine(std::make_shared<StubOcrEngine>(false));
    EXPECT_FALSE(mgr.IsAvailable());
}

TEST(OcrManager, GetEngineReturnsSet) {
    auto engine = std::make_shared<StubOcrEngine>();
    OcrManager mgr(engine);
    EXPECT_EQ(mgr.GetEngine(), engine);
}

TEST(OcrManager, RecognizeRegionCallsEngine) {
    auto stub = std::make_shared<StubOcrEngine>();
    OcrManager mgr(stub);
    OcrResult r = mgr.RecognizeRegion({ 0, 0, 800, 600 });
    EXPECT_TRUE(r.succeeded);
    EXPECT_EQ(stub->CallCount(), 1);
}

TEST(OcrManager, RecognizeRegionReturnsWords) {
    OcrManager mgr(std::make_shared<StubOcrEngine>());
    OcrResult r = mgr.RecognizeRegion({ 0, 0, 800, 600 });
    EXPECT_GT(r.WordCount(), 0u);
}

TEST(OcrManager, NoEnginereturnsEmptyResult) {
    OcrManager mgr;
    OcrResult r = mgr.RecognizeRegion({ 0, 0, 100, 100 });
    EXPECT_FALSE(r.succeeded);
    EXPECT_TRUE(r.IsEmpty());
}

TEST(OcrManager, CacheSetAfterRecognize) {
    OcrManager mgr(std::make_shared<StubOcrEngine>());
    EXPECT_FALSE(mgr.HasCachedResult());
    mgr.RecognizeRegion({ 0, 0, 100, 100 });
    EXPECT_TRUE(mgr.HasCachedResult());
}

TEST(OcrManager, GetLastResultMatchesRecognized) {
    OcrManager mgr(std::make_shared<StubOcrEngine>());
    OcrResult r1 = mgr.RecognizeRegion({ 0, 0, 100, 100 });
    OcrResult r2 = mgr.GetLastResult();
    EXPECT_EQ(r1.FullText(), r2.FullText());
}

TEST(OcrManager, ClearCacheRemovesCached) {
    OcrManager mgr(std::make_shared<StubOcrEngine>());
    mgr.RecognizeRegion({ 0, 0, 100, 100 });
    ASSERT_TRUE(mgr.HasCachedResult());
    mgr.ClearCache();
    EXPECT_FALSE(mgr.HasCachedResult());
}

TEST(OcrManager, SetEngineResetsCachedResult) {
    OcrManager mgr(std::make_shared<StubOcrEngine>());
    mgr.RecognizeRegion({ 0, 0, 100, 100 });
    ASSERT_TRUE(mgr.HasCachedResult());
    mgr.SetEngine(std::make_shared<StubOcrEngine>());
    EXPECT_FALSE(mgr.HasCachedResult());
}

TEST(OcrManager, RecognizeFromBitmapSucceeds) {
    OcrManager mgr(std::make_shared<StubOcrEngine>());
    OcrResult r = mgr.RecognizeFromBitmap(nullptr, {0,0,200,100});
    EXPECT_TRUE(r.succeeded);
}

TEST(OcrManager, RecognizeFromFileForwarded) {
    OcrManager mgr(std::make_shared<StubOcrEngine>());
    OcrResult r = mgr.RecognizeFromFile("test.png");
    EXPECT_TRUE(r.succeeded);
}

// ── Filter logic tests ────────────────────────────────────────────────────────

TEST(OcrManager, ConfidenceFilterRemovesLowConfidenceWords) {
    auto stub = std::make_shared<StubOcrEngine>();
    // Words: Hello=0.95, World=0.88
    stub->SetWords({ {"Hello", 0.95f}, {"World", 0.50f} });
    OcrManager mgr(stub);
    OcrOptions opts;
    opts.minConfidence = 0.80f;
    OcrResult r = mgr.RecognizeRegion({ 0,0,100,100 }, opts);
    // Only "Hello" survives
    EXPECT_EQ(r.WordCount(), 1u);
    EXPECT_EQ(r.lines[0].words[0].text, "Hello");
}

TEST(OcrManager, ConfidenceFilterZeroKeepsAll) {
    OcrManager mgr(std::make_shared<StubOcrEngine>());
    OcrOptions opts;
    opts.minConfidence = 0.0f;
    OcrResult r = mgr.RecognizeRegion({ 0,0,100,100 }, opts);
    EXPECT_EQ(r.WordCount(), 2u); // Hello + World
}

TEST(OcrManager, TrimWhitespaceStripsWords) {
    auto stub = std::make_shared<StubOcrEngine>();
    stub->SetWords({ {"  hi  ", 1.0f}, {"  ", 1.0f} });
    OcrManager mgr(stub);
    OcrOptions opts;
    opts.trimWhitespace = true;
    OcrResult r = mgr.RecognizeRegion({ 0,0,100,100 }, opts);
    // "  " becomes empty and is removed; "  hi  " becomes "hi"
    ASSERT_EQ(r.WordCount(), 1u);
    EXPECT_EQ(r.lines[0].words[0].text, "hi");
}

TEST(OcrManager, TrimFalsePreservesSpaces) {
    auto stub = std::make_shared<StubOcrEngine>();
    stub->SetWords({ {" hi ", 1.0f} });
    OcrManager mgr(stub);
    OcrOptions opts;
    opts.trimWhitespace = false;
    OcrResult r = mgr.RecognizeRegion({ 0,0,100,100 }, opts);
    ASSERT_EQ(r.WordCount(), 1u);
    EXPECT_EQ(r.lines[0].words[0].text, " hi ");
}

TEST(OcrManager, AllWordsFilteredLeavesEmptyResult) {
    auto stub = std::make_shared<StubOcrEngine>();
    stub->SetWords({ {"x", 0.1f} });
    OcrManager mgr(stub);
    OcrOptions opts;
    opts.minConfidence = 0.9f;
    OcrResult r = mgr.RecognizeRegion({ 0,0,100,100 }, opts);
    EXPECT_EQ(r.WordCount(), 0u);
    EXPECT_TRUE(r.lines.empty()); // empty lines pruned
}

TEST(OcrManager, LanguageTagPropagated) {
    OcrManager mgr(std::make_shared<StubOcrEngine>());
    OcrOptions opts;
    opts.languageTag = "fr-FR";
    OcrResult r = mgr.RecognizeRegion({ 0,0,100,100 }, opts);
    EXPECT_EQ(r.languageTag, "fr-FR");
}

// ── OcrOptions defaults test ──────────────────────────────────────────────────

TEST(OcrOptions, DefaultsAreReasonable) {
    OcrOptions opts;
    EXPECT_EQ(opts.languageTag, "en-US");
    EXPECT_FLOAT_EQ(opts.minConfidence, 0.0f);
    EXPECT_TRUE(opts.trimWhitespace);
}
