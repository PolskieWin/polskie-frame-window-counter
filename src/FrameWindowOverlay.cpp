#include "FrameWindowOverlay.hpp"
#include <fmt/format.h>
#include <cmath>

static FrameWindowOverlay* s_instance = nullptr;

static ccColor3B windowColor(double windowFrames) {
    int window = static_cast<int>(std::round(windowFrames));
    if (window >= 11) return { 150, 80, 255 };
    if (window >= 8) return { 100, 130, 255 };
    if (window >= 6) return { 60, 200, 255 };
    if (window >= 5) return { 50, 230, 180 };
    if (window >= 4) return { 80, 255, 100 };
    if (window >= 3) return { 255, 240, 50 };
    if (window >= 2) return { 255, 160, 40 };
    if (window >= 1) return { 255, 90, 40 };
    return { 255, 40, 40 };
}

FrameWindowOverlay* FrameWindowOverlay::get() {
    return s_instance;
}

FrameWindowOverlay::~FrameWindowOverlay() {
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

FrameWindowOverlay* FrameWindowOverlay::create() {
    auto ret = new FrameWindowOverlay();
    if (ret && ret->init()) {
        ret->autorelease();
        s_instance = ret;
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

CCLabelBMFont* FrameWindowOverlay::createPusabLabel(const char* text, float scale, ccColor3B color, float x, float y) {
    auto label = CCLabelBMFont::create(text, "bigFont.fnt");
    label->setScale(scale);
    label->setColor(color);
    label->setAnchorPoint({ 0.0f, 1.0f });
    label->setPosition({ x, y });
    this->addChild(label, 1);
    return label;
}

void FrameWindowOverlay::setupBuckets() {
    m_buckets.clear();

    m_buckets.push_back({ "10+:",  11, 9999, { 150, 80, 255 }, nullptr });
    m_buckets.push_back({ "8-10:", 8,  10,   { 100, 130, 255 }, nullptr });
    m_buckets.push_back({ "6-8:",  6,  7,    { 60, 200, 255 }, nullptr });
    m_buckets.push_back({ "5-6:",  5,  5,    { 50, 230, 180 }, nullptr });
    m_buckets.push_back({ "4-5:",  4,  4,    { 80, 255, 100 }, nullptr });
    m_buckets.push_back({ "3-4:",  3,  3,    { 255, 240, 50 }, nullptr });
    m_buckets.push_back({ "2-3:",  2,  2,    { 255, 160, 40 }, nullptr });
    m_buckets.push_back({ "1-2:",  1,  1,    { 255, 90, 40 }, nullptr });
    m_buckets.push_back({ "0-1:",  0,  0,    { 255, 40, 40 }, nullptr });
}

bool FrameWindowOverlay::init() {
    if (!CCLayer::init()) return false;

    auto mod = Mod::get();
    float scale = static_cast<float>(mod->getSettingValue<double>("overlay-scale"));
    int opacity = mod->getSettingValue<int64_t>("overlay-opacity");
    bool showTitle = mod->getSettingValue<bool>("show-title");

    setupBuckets();

    // Semi-transparent background (default opacity 0 = clean transparent like NaN GD)
    m_background = CCLayerColor::create(ccc4(0, 0, 0, static_cast<GLubyte>(opacity)));
    m_background->setVisible(opacity > 0);
    this->addChild(m_background, 0);

    // Optional Title header (Pusab font, default hidden)
    m_titleLabel = createPusabLabel("POLSKIE FWC", 0.32f * scale, { 255, 215, 0 }, 0, 0);
    m_titleLabel->setVisible(showTitle);

    // 1:1 NaN GD bucket labels in Pusab font
    for (auto& bucket : m_buckets) {
        std::string txt = fmt::format("{:<6}0", bucket.label);
        bucket.labelNode = createPusabLabel(txt.c_str(), 0.30f * scale, bucket.color, 0, 0);
    }

    // Last click timing & exact MS indicator (e.g. "FP: 1f (4.17ms)!" or "LAST: 2f (8.33ms)")
    m_lastClickLabel = createPusabLabel("LAST: --", 0.28f * scale, { 245, 245, 245 }, 0, 0);

    // Real-time CPS counter for fast spam & swift clicks
    m_cpsLabel = createPusabLabel("CPS: 0.0  MAX: 0.0", 0.27f * scale, { 120, 220, 255 }, 0, 0);

    // Mini gamemode and gravity multiplier status
    m_gravLabel = createPusabLabel("MODE: CUBE | GRAV: NORMAL", 0.26f * scale, { 150, 255, 180 }, 0, 0);

    // CBF Status (Frame Perfect!)
    m_cbfLabel = createPusabLabel("CBF: 0", 0.28f * scale, { 239, 79, 150 }, 0, 0);

    // Rings Frame Perfect
    m_ringLabel = createPusabLabel("RINGS FP: 0/0", 0.27f * scale, { 255, 200, 50 }, 0, 0);

    // MS & Hz stats (Razy / NaN GD style)
    m_msLabel = createPusabLabel("0.00ms", 0.27f * scale, { 243, 242, 246 }, 0, 0);
    m_hzLabel = createPusabLabel("0Hz", 0.27f * scale, { 245, 243, 246 }, 0, 0);

    // Gamemode & Total Clicks
    m_modeLabel = createPusabLabel("MODE: CUBE", 0.26f * scale, { 100, 255, 150 }, 0, 0);
    m_clicksLabel = createPusabLabel("CLICKS: 0", 0.26f * scale, { 200, 200, 200 }, 0, 0);

    updateDisplay();
    updatePosition();
    this->scheduleUpdate();

    return true;
}

void FrameWindowOverlay::triggerCBFFlash() {
    m_cbfFlashTimer = 1.0f;
}

void FrameWindowOverlay::spawnClickMarker(CCNode* worldLayer, CCPoint position, double windowFrames) {
    if (!worldLayer || !Mod::get()->getSettingValue<bool>("show-click-rings")) return;

    auto color = windowColor(windowFrames);
    auto marker = CCLayer::create();
    if (!marker) return;
    marker->setPosition(position);
    marker->setScale(0.8f);
    marker->setCascadeOpacityEnabled(true);

    auto ring = CCDrawNode::create();
    auto tint = ccc4f(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, 1.0f);
    ring->drawCircle({ 0.0f, 0.0f }, 13.0f, ccc4f(0.0f, 0.0f, 0.0f, 0.0f), 2.0f, tint);
    marker->addChild(ring);

    auto label = CCLabelBMFont::create(fmt::format("{:.1f}", windowFrames).c_str(), "bigFont.fnt");
    label->setScale(0.30f);
    label->setColor(color);
    label->setPosition({ 0.0f, 18.0f });
    marker->addChild(label);

    worldLayer->addChild(marker, 999);
    auto animation = CCSpawn::create(
        CCScaleTo::create(0.35f, 1.5f),
        CCFadeOut::create(0.35f),
        nullptr
    );
    marker->runAction(CCSequence::create(animation, CCRemoveSelf::create(), nullptr));
}

void FrameWindowOverlay::update(float dt) {
    if (m_cbfFlashTimer > 0.0f) {
        m_cbfFlashTimer -= dt;
        if (m_cbfFlashTimer < 0.0f) m_cbfFlashTimer = 0.0f;
    }

    m_updateTimer += dt;
    if (m_updateTimer >= m_updateInterval) {
        m_updateTimer = 0.0f;
        updateDisplay();
    }
}

void FrameWindowOverlay::updateDisplay() {
    auto analyzer = FrameWindowAnalyzer::get();
    auto mod = Mod::get();

    bool showTitle = mod->getSettingValue<bool>("show-title");
    bool showHigher = mod->getSettingValue<bool>("show-higher-windows");
    bool show10Plus = mod->getSettingValue<bool>("show-10plus-only");
    bool showCBF = mod->getSettingValue<bool>("show-cbf");
    bool showRings = mod->getSettingValue<bool>("show-ring-fp");
    bool showMsHz = mod->getSettingValue<bool>("show-ms-hz");
    bool showGamemode = mod->getSettingValue<bool>("show-gamemode");

    // New 99.9% precision & MS features
    bool showBucketMS = mod->getSettingValue<bool>("show-bucket-ms");
    bool showLastMS = mod->getSettingValue<bool>("show-last-ms");
    bool showCPS = mod->getSettingValue<bool>("show-cps");
    bool showGravityMini = mod->getSettingValue<bool>("show-gravity-mini");

    if (m_titleLabel) m_titleLabel->setVisible(showTitle);

    auto const& dist = analyzer->getWindowDistribution();

    // Update each bucket
    for (auto& bucket : m_buckets) {
        if (!bucket.labelNode) continue;

        // Higher window filter (8+)
        if (!showHigher && bucket.minFrames >= 8) {
            bucket.labelNode->setVisible(false);
            continue;
        }

        if (!show10Plus && bucket.minFrames >= 11) {
            bucket.labelNode->setVisible(false);
            continue;
        }

        bucket.labelNode->setVisible(true);

        // Count clicks falling into this bucket
        int count = 0;
        for (auto const& [win, c] : dist) {
            if (win >= bucket.minFrames && win <= bucket.maxFrames) count += c;
        }

        if (showBucketMS) {
            double ms = (bucket.minFrames >= 11)
                ? analyzer->framesToMS(11)
                : analyzer->framesToMS(bucket.maxFrames);
            std::string msStr = (bucket.minFrames >= 11)
                ? fmt::format("{:.1f}ms+", ms)
                : fmt::format("{:.1f}ms", ms);
            bucket.labelNode->setString(fmt::format("{:<6}{} ({})", bucket.label, count, msStr).c_str());
        } else {
            bucket.labelNode->setString(fmt::format("{:<6}{}", bucket.label, count).c_str());
        }

        // Bright color on non-zero, slightly dimmed on zero (like NaN GD)
        if (count > 0) {
            bucket.labelNode->setColor(bucket.color);
        } else {
            bucket.labelNode->setColor({
                static_cast<GLubyte>(bucket.color.r * 2 / 3),
                static_cast<GLubyte>(bucket.color.g * 2 / 3),
                static_cast<GLubyte>(bucket.color.b * 2 / 3)
            });
        }
    }

    // Last click MS & timing indicator
    if (m_lastClickLabel) {
        m_lastClickLabel->setVisible(showLastMS);
        if (showLastMS) {
            m_lastClickLabel->setString(analyzer->getLastClickText().c_str());
            if (analyzer->getLastClickWindow() <= 1 && analyzer->getTotalClicks() > 0) {
                m_lastClickLabel->setColor({ 255, 255, 0 }); // Bright Yellow Frame Perfect!
            } else {
                m_lastClickLabel->setColor({ 245, 245, 245 });
            }
        }
    }

    // Real-time CPS for fast spam & swift clicks
    if (m_cpsLabel) {
        m_cpsLabel->setVisible(showCPS);
        if (showCPS) {
            m_cpsLabel->setString(analyzer->getCPSText().c_str());
            if (analyzer->getCurrentCPS() >= 12.0) {
                m_cpsLabel->setColor({ 255, 120, 50 }); // Orange for fast spam bursts!
            } else {
                m_cpsLabel->setColor({ 120, 220, 255 });
            }
        }
    }

    // Gamemode, Mini Mode & Gravity status
    if (m_gravLabel) {
        m_gravLabel->setVisible(showGravityMini);
        if (showGravityMini) {
            m_gravLabel->setString(analyzer->getGravityMiniText().c_str());
        }
    }

    // MS and Hz stats
    if (showMsHz) {
        m_msLabel->setVisible(true);
        m_hzLabel->setVisible(true);

        double ms = analyzer->getLastClickMS();
        double effHz = (ms > 0.0) ? (1000.0 / ms) : 0.0;

        m_msLabel->setString(fmt::format("{:.2f}ms", ms).c_str());
        m_hzLabel->setString(fmt::format("{:.0f}Hz", effHz).c_str());
    } else {
        m_msLabel->setVisible(false);
        m_hzLabel->setVisible(false);
    }

    // CBF Status (Frame Perfect!)
    if (showCBF) {
        m_cbfLabel->setVisible(true);
        int cbfCount = analyzer->getCBFCount();

        if (analyzer->isLastClickCBF() || m_cbfFlashTimer > 0.0f) {
            m_cbfLabel->setString(fmt::format("CBF: {} (Frame Perfect!)", cbfCount).c_str());
            m_cbfLabel->setColor({ 255, 255, 0 }); // Bright Yellow flash!
        } else {
            m_cbfLabel->setString(fmt::format("CBF: {}", cbfCount).c_str());
            m_cbfLabel->setColor({ 239, 79, 150 }); // Frame perfect pink
        }
    } else {
        m_cbfLabel->setVisible(false);
    }

    // Ring Frame Perfect
    if (showRings) {
        m_ringLabel->setVisible(true);
        int ringFP = analyzer->getRingFPCount();
        int totalRings = analyzer->getTotalRingHits();
        m_ringLabel->setString(fmt::format("RINGS FP: {}/{}", ringFP, totalRings).c_str());
    } else {
        m_ringLabel->setVisible(false);
    }

    // Gamemode
    if (showGamemode) {
        m_modeLabel->setVisible(true);
        std::string modeStr = FrameWindowAnalyzer::gamemodeName(analyzer->getCurrentGamemode(), analyzer->getGameState().isMini);
        std::transform(modeStr.begin(), modeStr.end(), modeStr.begin(), ::toupper);
        m_modeLabel->setString(fmt::format("MODE: {}", modeStr).c_str());
    } else {
        m_modeLabel->setVisible(false);
    }

    // Total clicks
    m_clicksLabel->setString(fmt::format("CLICKS: {}", analyzer->getTotalClicks()).c_str());
}

void FrameWindowOverlay::updatePosition() {
    auto mod = Mod::get();
    std::string position = mod->getSettingValue<std::string>("overlay-position");
    float scale = static_cast<float>(mod->getSettingValue<double>("overlay-scale"));
    bool showTitle = mod->getSettingValue<bool>("show-title");
    bool showMsHz = mod->getSettingValue<bool>("show-ms-hz");
    bool showCBF = mod->getSettingValue<bool>("show-cbf");
    bool showRings = mod->getSettingValue<bool>("show-ring-fp");
    bool showGamemode = mod->getSettingValue<bool>("show-gamemode");
    bool showLastMS = mod->getSettingValue<bool>("show-last-ms");
    bool showCPS = mod->getSettingValue<bool>("show-cps");
    bool showGravityMini = mod->getSettingValue<bool>("show-gravity-mini");
    int opacity = mod->getSettingValue<int64_t>("overlay-opacity");

    auto winSize = CCDirector::get()->getWinSize();

    // Scale adjustments for Pusab font
    if (m_titleLabel) m_titleLabel->setScale(0.32f * scale);
    for (auto& bucket : m_buckets) {
        if (bucket.labelNode) bucket.labelNode->setScale(0.30f * scale);
    }
    if (m_lastClickLabel) m_lastClickLabel->setScale(0.28f * scale);
    if (m_cpsLabel) m_cpsLabel->setScale(0.27f * scale);
    if (m_gravLabel) m_gravLabel->setScale(0.26f * scale);
    if (m_cbfLabel) m_cbfLabel->setScale(0.28f * scale);
    if (m_ringLabel) m_ringLabel->setScale(0.27f * scale);
    if (m_msLabel) m_msLabel->setScale(0.27f * scale);
    if (m_hzLabel) m_hzLabel->setScale(0.27f * scale);
    if (m_modeLabel) m_modeLabel->setScale(0.26f * scale);
    if (m_clicksLabel) m_clicksLabel->setScale(0.26f * scale);

    // Collect visible labels in order from top to bottom
    std::vector<CCLabelBMFont*> visibleLabels;
    if (showTitle && m_titleLabel && m_titleLabel->isVisible()) visibleLabels.push_back(m_titleLabel);

    for (auto& b : m_buckets) {
        if (b.labelNode && b.labelNode->isVisible()) {
            visibleLabels.push_back(b.labelNode);
        }
    }

    if (showMsHz && m_msLabel && m_msLabel->isVisible()) {
        visibleLabels.push_back(m_msLabel);
        if (m_hzLabel && m_hzLabel->isVisible()) visibleLabels.push_back(m_hzLabel);
    }
    if (showLastMS && m_lastClickLabel && m_lastClickLabel->isVisible()) visibleLabels.push_back(m_lastClickLabel);
    if (showCPS && m_cpsLabel && m_cpsLabel->isVisible()) visibleLabels.push_back(m_cpsLabel);
    if (showGravityMini && m_gravLabel && m_gravLabel->isVisible()) visibleLabels.push_back(m_gravLabel);
    if (showCBF && m_cbfLabel && m_cbfLabel->isVisible()) visibleLabels.push_back(m_cbfLabel);
    if (showRings && m_ringLabel && m_ringLabel->isVisible()) visibleLabels.push_back(m_ringLabel);
    if (showGamemode && m_modeLabel && m_modeLabel->isVisible()) visibleLabels.push_back(m_modeLabel);
    if (m_clicksLabel && m_clicksLabel->isVisible()) visibleLabels.push_back(m_clicksLabel);

    if (visibleLabels.empty()) {
        if (m_background) m_background->setVisible(false);
        return;
    }

    // Precise line height matching Pusab glyph metric
    float lineH = 11.5f * scale;

    bool isRight = (position == "top-right" || position == "bottom-right");
    bool isBottom = (position == "bottom-left" || position == "bottom-right");

    // Strictly pinned to top-left screen corner
    float startX = isRight ? (winSize.width - 8.0f) : 8.0f;
    float startY = isBottom ? (8.0f + (visibleLabels.size() * lineH)) : (winSize.height - 8.0f);

    CCPoint anchor = isRight ? CCPoint{ 1.0f, 1.0f } : CCPoint{ 0.0f, 1.0f };

    float curY = startY;
    float maxW = 0.0f;

    for (auto* lbl : visibleLabels) {
        lbl->setAnchorPoint(anchor);
        lbl->setPosition({ startX, curY });
        float w = lbl->getContentWidth() * lbl->getScale();
        if (w > maxW) maxW = w;
        curY -= lineH;
    }

    // Optional background rect behind text
    if (m_background) {
        if (opacity > 0) {
            m_background->setVisible(true);
            m_background->setOpacity(static_cast<GLubyte>(opacity));
            float totalH = (visibleLabels.size() * lineH) + 6.0f;
            float totalW = maxW + 10.0f;
            m_background->setContentSize({ totalW, totalH });
            m_background->ignoreAnchorPointForPosition(false);
            m_background->setAnchorPoint(anchor);
            m_background->setPosition({ startX + (isRight ? 4.0f : -4.0f), startY + 3.0f });
        } else {
            m_background->setVisible(false);
        }
    }
}

void FrameWindowOverlay::refreshSettings() {
    updateDisplay();
    updatePosition();
}
