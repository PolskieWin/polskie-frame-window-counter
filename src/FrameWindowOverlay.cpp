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

    // Razy's Frame Windows & NaN GD buckets
    // High to low (rendered top to bottom)
    m_buckets.push_back({ "10+:",  10, 999, { 190, 70,  255 }, nullptr }); // Purple
    m_buckets.push_back({ "8-10:", 8,  10,  { 70,  120, 255 }, nullptr }); // Blue
    m_buckets.push_back({ "6-8:",  6,  8,   { 70,  190, 255 }, nullptr }); // Light Blue
    m_buckets.push_back({ "5-6:",  5,  6,   { 0,   255, 235 }, nullptr }); // Cyan
    m_buckets.push_back({ "4-5:",  4,  5,   { 50,  255, 90  }, nullptr }); // Green
    m_buckets.push_back({ "3-4:",  3,  4,   { 255, 240, 20  }, nullptr }); // Yellow
    m_buckets.push_back({ "2-3:",  2,  3,   { 255, 140, 20  }, nullptr }); // Orange
    m_buckets.push_back({ "1-2:",  1,  2,   { 255, 40,  40  }, nullptr }); // Red
    m_buckets.push_back({ "0-1:",  0,  1,   { 255, 20,  130 }, nullptr }); // Pink/Magenta = FRAME PERFECT!
}

bool FrameWindowOverlay::init() {
    if (!CCLayer::init()) return false;

    auto mod = Mod::get();
    float scale = static_cast<float>(mod->getSettingValue<double>("overlay-scale"));
    int opacity = mod->getSettingValue<int64_t>("overlay-opacity");

    setupBuckets();

    // Semi-transparent background
    float totalW = 195.0f;
    float totalH = 265.0f;
    m_background = CCLayerColor::create(ccc4(0, 0, 0, static_cast<GLubyte>(opacity)));
    m_background->setContentSize({ totalW, totalH });
    m_background->setScale(scale);
    this->addChild(m_background, 0);

    float xPad = 8.0f;
    float yPos = totalH - 14.0f;

    // Title: Polskie FWC in gold Pusab font
    m_titleLabel = createPusabLabel("POLSKIE FWC", 0.38f, { 255, 215, 0 }, xPad, yPos);
    yPos -= 14.0f;

    // Create labels for all buckets in Pusab font
    for (auto& bucket : m_buckets) {
        std::string txt = fmt::format("{:<5} 0", bucket.label);
        bucket.labelNode = createPusabLabel(txt.c_str(), 0.33f, bucket.color, xPad, yPos);
        yPos -= 12.5f;
    }

    yPos -= 3.0f;

    // MS & Hz stats (from the video!)
    m_msLabel = createPusabLabel("0.00MS", 0.30f, { 255, 255, 255 }, xPad, yPos);
    yPos -= 11.5f;

    m_hzLabel = createPusabLabel("240Hz", 0.30f, { 255, 255, 255 }, xPad, yPos);
    yPos -= 14.0f;

    // CBF Status (Frame Perfect!)
    m_cbfLabel = createPusabLabel("CBF: 0", 0.31f, { 255, 80, 180 }, xPad, yPos);
    yPos -= 12.0f;

    // Rings Frame Perfect
    m_ringLabel = createPusabLabel("RINGS FP: 0/0", 0.28f, { 255, 200, 50 }, xPad, yPos);
    yPos -= 11.5f;

    // Mode & Total Clicks
    m_modeLabel = createPusabLabel("MODE: CUBE", 0.27f, { 100, 255, 150 }, xPad, yPos);
    yPos -= 11.0f;

    m_clicksLabel = createPusabLabel("CLICKS: 0", 0.27f, { 200, 200, 200 }, xPad, yPos);

    updatePosition();
    this->scheduleUpdate();

    return true;
}

void FrameWindowOverlay::triggerCBFFlash() {
    m_cbfFlashTimer = 1.0f; // Flash for 1 second
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

    bool showHigher = mod->getSettingValue<bool>("show-higher-windows");
    bool show10Plus = mod->getSettingValue<bool>("show-10plus-only");
    bool showCBF = mod->getSettingValue<bool>("show-cbf");
    bool showRings = mod->getSettingValue<bool>("show-ring-fp");
    bool showMsHz = mod->getSettingValue<bool>("show-ms-hz");
    bool showGamemode = mod->getSettingValue<bool>("show-gamemode");

    auto const& dist = analyzer->getWindowDistribution();

    // Update each bucket
    for (auto& bucket : m_buckets) {
        if (!bucket.labelNode) continue;

        // Check if higher windows should be hidden
        if (!showHigher && (bucket.minFrames >= 8)) {
            bucket.labelNode->setVisible(false);
            continue;
        }
        if (!show10Plus && (bucket.minFrames >= 10)) {
            bucket.labelNode->setVisible(false);
            continue;
        }

        bucket.labelNode->setVisible(true);

        // Count clicks falling into this bucket
        int count = 0;
        for (auto const& [win, c] : dist) {
            if (bucket.maxFrames >= 999) {
                if (win >= bucket.minFrames) count += c;
            } else {
                if (win >= bucket.minFrames && win < bucket.maxFrames) count += c;
            }
        }

        bucket.labelNode->setString(fmt::format("{:<5} {}", bucket.label, count).c_str());

        // Make non-zero numbers pop with full color, zero numbers slightly dimmed
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

    // MS and Hz calculations (Razy's style)
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
    // When last click was CBF or flash timer is active, highlight!
    if (showCBF) {
        m_cbfLabel->setVisible(true);
        int cbfCount = analyzer->getCBFCount();

        if (analyzer->isLastClickCBF() || m_cbfFlashTimer > 0.0f) {
            // "CBF: X (Frame Perfect!)" in bright flashing yellow/pink
            m_cbfLabel->setString(fmt::format("CBF: {} (Frame Perfect!)", cbfCount).c_str());
            m_cbfLabel->setColor({ 255, 255, 0 }); // Bright Yellow flash!
            m_cbfLabel->setScale(0.33f);
        } else {
            // Normal state: "CBF: X"
            m_cbfLabel->setString(fmt::format("CBF: {}", cbfCount).c_str());
            m_cbfLabel->setColor({ 255, 60, 160 }); // Classic pink
            m_cbfLabel->setScale(0.30f);
        }
    } else {
        m_cbfLabel->setVisible(false);
    }

    // Rings Frame Perfect
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
