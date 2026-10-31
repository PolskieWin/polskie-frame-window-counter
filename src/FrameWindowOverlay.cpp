#include "FrameWindowOverlay.hpp"
#include <fmt/format.h>

FrameWindowOverlay* FrameWindowOverlay::create() {
    auto ret = new FrameWindowOverlay();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

CCLabelBMFont* FrameWindowOverlay::createPusabLabel(const char* text, float scale, ccColor3B color, float x, float y) {
    auto label = CCLabelBMFont::create(text, "bigFont.fnt");
    label->setScale(scale);
    label->setColor(color);
    label->setAnchorPoint({ 0.0f, 0.5f });
    label->setPosition({ x, y });
    m_background->addChild(label);
    return label;
}

void FrameWindowOverlay::setupBuckets() {
    m_buckets.clear();

    // 1:1 NaN GD bucket layout & exact sampled colors
    m_buckets.push_back({ "10+:",  11, 9999, { 186, 93,  249 }, nullptr }); // #BA5DF9 Violet
    m_buckets.push_back({ "9-10:", 9,  10,   { 138, 95,  249 }, nullptr }); // #8A5FF9 Purple
    m_buckets.push_back({ "7-8:",  7,  8,    { 89,  201, 215 }, nullptr }); // #59C9D7 Light Blue / Cyan
    m_buckets.push_back({ "5-6:",  5,  6,    { 119, 223, 145 }, nullptr }); // #77DF91 Lime / Mint Green
    m_buckets.push_back({ "3-4:",  3,  4,    { 208, 213, 59  }, nullptr }); // #D0D53B Pastel Yellow
    m_buckets.push_back({ "1-2:",  1,  2,    { 232, 160, 32  }, nullptr }); // #E8A020 Amber / Orange
    m_buckets.push_back({ "0-1:",  0,  1,    { 239, 79,  80  }, nullptr }); // #EF4F50 Frame Perfect Pink/Red
}

bool FrameWindowOverlay::init() {
    if (!CCLayer::init()) return false;

    auto mod = Mod::get();
    float scale = static_cast<float>(mod->getSettingValue<double>("overlay-scale"));
    int opacity = mod->getSettingValue<int64_t>("overlay-opacity");
    bool showTitle = mod->getSettingValue<bool>("show-title");

    setupBuckets();

    // Semi-transparent background (default opacity 0 = clean transparent like NaN GD)
    float totalW = 180.0f;
    float totalH = showTitle ? 250.0f : 235.0f;
    m_background = CCLayerColor::create(ccc4(0, 0, 0, static_cast<GLubyte>(opacity)));
    m_background->setContentSize({ totalW, totalH });
    m_background->setScale(scale);
    this->addChild(m_background, 0);

    float xPad = 6.0f;
    float yPos = totalH - 12.0f;

    // Optional Title header (Pusab font)
    m_titleLabel = createPusabLabel("POLSKIE FWC", 0.35f, { 255, 215, 0 }, xPad, yPos);
    m_titleLabel->setVisible(showTitle);
    if (showTitle) {
        yPos -= 14.0f;
    }

    // 1:1 NaN GD bucket labels in Pusab font
    for (auto& bucket : m_buckets) {
        std::string txt = fmt::format("{:<5} 0", bucket.label);
        bucket.labelNode = createPusabLabel(txt.c_str(), 0.33f, bucket.color, xPad, yPos);
        yPos -= 12.5f;
    }

    yPos -= 4.0f;

    // MS & Hz stats (Razy / NaN GD style)
    m_msLabel = createPusabLabel("0.00MS", 0.30f, { 243, 242, 246 }, xPad, yPos);
    yPos -= 11.5f;

    m_hzLabel = createPusabLabel("240Hz", 0.30f, { 245, 243, 246 }, xPad, yPos);
    yPos -= 13.0f;

    // CBF Status (Frame Perfect!)
    m_cbfLabel = createPusabLabel("CBF: 0", 0.30f, { 239, 79, 150 }, xPad, yPos);
    yPos -= 12.0f;

    // Rings Frame Perfect
    m_ringLabel = createPusabLabel("RINGS FP: 0/0", 0.28f, { 255, 200, 50 }, xPad, yPos);
    yPos -= 11.5f;

    // Gamemode & Total Clicks
    m_modeLabel = createPusabLabel("MODE: CUBE", 0.27f, { 100, 255, 150 }, xPad, yPos);
    yPos -= 11.0f;

    m_clicksLabel = createPusabLabel("CLICKS: 0", 0.27f, { 200, 200, 200 }, xPad, yPos);

    updatePosition();
    this->scheduleUpdate();

    return true;
}

void FrameWindowOverlay::triggerCBFFlash() {
    m_cbfFlashTimer = 1.0f;
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

    if (m_titleLabel) m_titleLabel->setVisible(showTitle);

    auto const& dist = analyzer->getWindowDistribution();

    // Update each bucket
    for (auto& bucket : m_buckets) {
        if (!bucket.labelNode) continue;

        // Higher window filter (9-10 and 10+)
        if (!showHigher && (bucket.minFrames >= 9)) {
            bucket.labelNode->setVisible(false);
            continue;
        }
        if (!show10Plus && (bucket.minFrames >= 11)) {
            bucket.labelNode->setVisible(false);
            continue;
        }

        bucket.labelNode->setVisible(true);

        // Count clicks falling into this bucket
        int count = 0;
        for (auto const& [win, c] : dist) {
            if (bucket.minFrames >= 11) {
                if (win >= 11) count += c;
            } else if (bucket.label == "9-10:") {
                if (win == 9 || win == 10) count += c;
            } else if (bucket.label == "7-8:") {
                if (win == 7 || win == 8) count += c;
            } else if (bucket.label == "5-6:") {
                if (win == 5 || win == 6) count += c;
            } else if (bucket.label == "3-4:") {
                if (win == 3 || win == 4) count += c;
            } else if (bucket.label == "1-2:") {
                if (win == 1 || win == 2) count += c;
            } else if (bucket.label == "0-1:") {
                if (win == 0 || win == 1) count += c;
            }
        }

        bucket.labelNode->setString(fmt::format("{:<5} {}", bucket.label, count).c_str());

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

    // MS and Hz stats
    if (showMsHz) {
        m_msLabel->setVisible(true);
        m_hzLabel->setVisible(true);

        double avgWindow = analyzer->getAverageWindow();
        int fps = analyzer->getAnalysisFPS();
        double ms = (avgWindow > 0.0) ? (avgWindow / static_cast<double>(fps)) * 1000.0 : (1000.0 / fps);
        double effHz = (ms > 0.0) ? (1000.0 / ms) : static_cast<double>(fps);

        m_msLabel->setString(fmt::format("{:.2f}MS", ms).c_str());
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
            m_cbfLabel->setScale(0.32f);
        } else {
            m_cbfLabel->setString(fmt::format("CBF: {}", cbfCount).c_str());
            m_cbfLabel->setColor({ 239, 79, 150 }); // Frame perfect pink
            m_cbfLabel->setScale(0.30f);
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
        std::string modeStr = FrameWindowAnalyzer::gamemodeName(analyzer->getCurrentGamemode());
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

    auto winSize = CCDirector::get()->getWinSize();
    float bgW = m_background->getContentWidth() * scale;
    float bgH = m_background->getContentHeight() * scale;

    float x = 4.0f;
    float y = 4.0f;

    if (position == "top-left") {
        x = 4.0f;
        y = winSize.height - bgH - 4.0f;
    } else if (position == "top-right") {
        x = winSize.width - bgW - 4.0f;
        y = winSize.height - bgH - 4.0f;
    } else if (position == "bottom-left") {
        x = 4.0f;
        y = 4.0f;
    } else if (position == "bottom-right") {
        x = winSize.width - bgW - 4.0f;
        y = 4.0f;
    }

    m_background->setPosition({ x, y });
}

void FrameWindowOverlay::refreshSettings() {
    auto mod = Mod::get();
    float scale = static_cast<float>(mod->getSettingValue<double>("overlay-scale"));
    int opacity = mod->getSettingValue<int64_t>("overlay-opacity");

    m_background->setScale(scale);
    m_background->setOpacity(static_cast<GLubyte>(opacity));
    updatePosition();
}
