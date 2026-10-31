#include "FWCSettingsPopup.hpp"
#include "FrameWindowOverlay.hpp"
#include <fmt/format.h>
#include <Geode/utils/file.hpp>
#include <Geode/utils/async.hpp>

FWCSettingsPopup* FWCSettingsPopup::create() {
    auto ret = new FWCSettingsPopup();
    if (ret && ret->init(420.f, 325.f)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

CCMenuItemToggler* FWCSettingsPopup::createRobTopToggle(
    std::string const& label, bool defaultVal,
    SEL_MenuHandler callback, float x, float y
) {
    auto onSpr = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");
    auto offSpr = CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");
    onSpr->setScale(0.52f);
    offSpr->setScale(0.52f);

    auto toggle = CCMenuItemToggler::create(offSpr, onSpr, this, callback);
    toggle->toggle(defaultVal);
    toggle->setPosition({ x, y });
    m_mainMenu->addChild(toggle);

    // Pusab font for label
    auto lbl = CCLabelBMFont::create(label.c_str(), "bigFont.fnt");
    lbl->setScale(0.23f);
    lbl->setAnchorPoint({ 0.f, 0.5f });
    lbl->setPosition({ x + 14.f, y });
    m_mainLayer->addChild(lbl);

    return toggle;
}

bool FWCSettingsPopup::init(float width, float height) {
    if (!Popup::init(width, height)) return false;

    auto mod = Mod::get();

    this->setTitle("Polskie Frame Windows");

    m_mainMenu = CCMenu::create();
    m_mainMenu->setPosition({ 0.f, 0.f });
    m_mainLayer->addChild(m_mainMenu);

    float centerX = width / 2.0f;
    float startY = height - 42.0f;
    float rowH = 19.5f;

    // ── Row 1: Enable & Status ──
    m_enabledToggle = createRobTopToggle(
        "Enabled", mod->getSettingValue<bool>("enabled"),
        menu_selector(FWCSettingsPopup::onToggleEnabled),
        35.f, startY
    );

    m_statusLabel = CCLabelBMFont::create("Made by Polskie", "goldFont.fnt");
    m_statusLabel->setScale(0.33f);
    m_statusLabel->setAnchorPoint({ 1.0f, 0.5f });
    m_statusLabel->setPosition({ width - 35.f, startY });
    m_mainLayer->addChild(m_statusLabel);
    startY -= rowH;

    // ── Section: Features ──
    auto featTitle = CCLabelBMFont::create("-- 1:1 HUD & PRECISE MS --", "goldFont.fnt");
    featTitle->setScale(0.30f);
    featTitle->setPosition({ centerX, startY });
    m_mainLayer->addChild(featTitle);
    startY -= rowH - 2.f;

    // Row 2: Sounds & CBF
    m_soundToggle = createRobTopToggle(
        "FP Sound (NaN GD)", mod->getSettingValue<bool>("fp-sound-enabled"),
        menu_selector(FWCSettingsPopup::onToggleSound),
        35.f, startY
    );

    m_cbfToggle = createRobTopToggle(
        "CBF Indicator", mod->getSettingValue<bool>("show-cbf"),
        menu_selector(FWCSettingsPopup::onToggleCBF),
        220.f, startY
    );
    startY -= rowH;

    // Row 3: Ring FP & Bucket MS (e.g. 8.33ms)
    m_ringToggle = createRobTopToggle(
        "Ring / Orb FP", mod->getSettingValue<bool>("show-ring-fp"),
        menu_selector(FWCSettingsPopup::onToggleRing),
        35.f, startY
    );

    m_bucketMSToggle = createRobTopToggle(
        "Bucket MS (8.33ms)", mod->getSettingValue<bool>("show-bucket-ms"),
        menu_selector(FWCSettingsPopup::onToggleBucketMS),
        220.f, startY
    );
    startY -= rowH;

    // Row 4: Last Click MS Timing & CPS
    m_lastMSToggle = createRobTopToggle(
        "Last Click MS", mod->getSettingValue<bool>("show-last-ms"),
        menu_selector(FWCSettingsPopup::onToggleLastMS),
        35.f, startY
    );

    m_cpsToggle = createRobTopToggle(
        "CPS (Fast Spam)", mod->getSettingValue<bool>("show-cps"),
        menu_selector(FWCSettingsPopup::onToggleCPS),
        220.f, startY
    );
    startY -= rowH;

    // Row 5: Mini & Gravity & Higher Windows
    m_gravityMiniToggle = createRobTopToggle(
        "Mini & Gravity", mod->getSettingValue<bool>("show-gravity-mini"),
        menu_selector(FWCSettingsPopup::onToggleGravityMini),
        35.f, startY
    );

    m_higherToggle = createRobTopToggle(
        "Higher Win (9-10)", mod->getSettingValue<bool>("show-higher-windows"),
        menu_selector(FWCSettingsPopup::onToggleHigher),
        220.f, startY
    );
    startY -= rowH;

    // Row 6: 10+ Bucket & Show MS/Hz
    m_10plusToggle = createRobTopToggle(
        "Show 10+ Bucket", mod->getSettingValue<bool>("show-10plus-only"),
        menu_selector(FWCSettingsPopup::onToggle10Plus),
        35.f, startY
    );

    m_msHzToggle = createRobTopToggle(
        "Show MS & Hz", mod->getSettingValue<bool>("show-ms-hz"),
        menu_selector(FWCSettingsPopup::onToggleMsHz),
        220.f, startY
    );
    startY -= rowH + 2.f;

    // ── FPS Selector ──
    auto fpsTitle = CCLabelBMFont::create("Analysis FPS:", "bigFont.fnt");
    fpsTitle->setScale(0.24f);
    fpsTitle->setAnchorPoint({ 0.f, 0.5f });
    fpsTitle->setPosition({ 35.f, startY });
    m_mainLayer->addChild(fpsTitle);

    auto leftSpr = CCSprite::createWithSpriteFrameName("edit_leftBtn_001.png");
    leftSpr->setScale(0.52f);
    auto leftBtn = CCMenuItemSpriteExtra::create(leftSpr, this, menu_selector(FWCSettingsPopup::onDecreaseFPS));
    leftBtn->setPosition({ 150.f, startY });
    m_mainMenu->addChild(leftBtn);

    int curFps = static_cast<int>(mod->getSettingValue<int64_t>("analysis-fps"));
    m_fpsLabel = CCLabelBMFont::create(fmt::format("{}Hz", curFps).c_str(), "bigFont.fnt");
    m_fpsLabel->setScale(0.30f);
    m_fpsLabel->setPosition({ 185.f, startY });
    m_mainLayer->addChild(m_fpsLabel);

    auto rightSpr = CCSprite::createWithSpriteFrameName("edit_rightBtn_001.png");
    rightSpr->setScale(0.52f);
    auto rightBtn = CCMenuItemSpriteExtra::create(rightSpr, this, menu_selector(FWCSettingsPopup::onIncreaseFPS));
    rightBtn->setPosition({ 220.f, startY });
    m_mainMenu->addChild(rightBtn);

    startY -= rowH + 4.f;

    // ── Section: Bot Macro Import ──
    auto macroTitle = CCLabelBMFont::create("-- BOT MACRO IMPORT (ECLIPSE / MEGAHACK / JSON) --", "goldFont.fnt");
    macroTitle->setScale(0.26f);
    macroTitle->setPosition({ centerX, startY });
    m_mainLayer->addChild(macroTitle);
    startY -= rowH - 2.f;

    m_macroInfoLabel = CCLabelBMFont::create("No macro loaded", "chatFont.fnt");
    m_macroInfoLabel->setScale(0.40f);
    m_macroInfoLabel->setPosition({ centerX, startY });
    m_macroInfoLabel->setColor({ 180, 180, 180 });
    m_mainLayer->addChild(m_macroInfoLabel);

    auto analyzer = FrameWindowAnalyzer::get();
    if (analyzer->hasMacroLoaded()) {
        m_macroInfoLabel->setString(
            fmt::format("Loaded: {} ({} clicks)", analyzer->getMacroName(), analyzer->getTotalClicks()).c_str()
        );
        m_macroInfoLabel->setColor({ 0, 255, 120 });
    }
    startY -= rowH + 2.f;

    // Buttons
    auto pickSpr = ButtonSprite::create("Select File...", "bigFont.fnt", "GJ_button_01.png", 0.58f);
    auto pickBtn = CCMenuItemSpriteExtra::create(pickSpr, this, menu_selector(FWCSettingsPopup::onPickMacroFile));
    pickBtn->setPosition({ 75.f, startY });
    pickBtn->setScale(0.65f);
    m_mainMenu->addChild(pickBtn);

    auto scanSpr = ButtonSprite::create("Scan Folder", "bigFont.fnt", "GJ_button_02.png", 0.58f);
    auto scanBtn = CCMenuItemSpriteExtra::create(scanSpr, this, menu_selector(FWCSettingsPopup::onScanMacroFolder));
    scanBtn->setPosition({ 180.f, startY });
    scanBtn->setScale(0.65f);
    m_mainMenu->addChild(scanBtn);

    auto clearSpr = ButtonSprite::create("Clear", "bigFont.fnt", "GJ_button_04.png", 0.58f);
    auto clearBtn = CCMenuItemSpriteExtra::create(clearSpr, this, menu_selector(FWCSettingsPopup::onClearMacro));
    clearBtn->setPosition({ 275.f, startY });
    clearBtn->setScale(0.65f);
    m_mainMenu->addChild(clearBtn);

    auto resetSpr = ButtonSprite::create("Reset", "bigFont.fnt", "GJ_button_06.png", 0.58f);
    auto resetBtn = CCMenuItemSpriteExtra::create(resetSpr, this, menu_selector(FWCSettingsPopup::onResetData));
    resetBtn->setPosition({ 350.f, startY });
    resetBtn->setScale(0.65f);
    m_mainMenu->addChild(resetBtn);

    return true;
}

void FWCSettingsPopup::onToggleEnabled(CCObject*) {
    auto mod = Mod::get();
    mod->setSettingValue("enabled", !mod->getSettingValue<bool>("enabled"));
    if (auto overlay = FrameWindowOverlay::get()) overlay->refreshSettings();
}

void FWCSettingsPopup::onToggleSound(CCObject*) {
    auto mod = Mod::get();
    mod->setSettingValue("fp-sound-enabled", !mod->getSettingValue<bool>("fp-sound-enabled"));
    if (auto overlay = FrameWindowOverlay::get()) overlay->refreshSettings();
}

void FWCSettingsPopup::onToggleCBF(CCObject*) {
    auto mod = Mod::get();
    mod->setSettingValue("show-cbf", !mod->getSettingValue<bool>("show-cbf"));
    if (auto overlay = FrameWindowOverlay::get()) overlay->refreshSettings();
}

void FWCSettingsPopup::onToggleRing(CCObject*) {
    auto mod = Mod::get();
    mod->setSettingValue("show-ring-fp", !mod->getSettingValue<bool>("show-ring-fp"));
    if (auto overlay = FrameWindowOverlay::get()) overlay->refreshSettings();
}

void FWCSettingsPopup::onToggleHigher(CCObject*) {
    auto mod = Mod::get();
    mod->setSettingValue("show-higher-windows", !mod->getSettingValue<bool>("show-higher-windows"));
    if (auto overlay = FrameWindowOverlay::get()) overlay->refreshSettings();
}

void FWCSettingsPopup::onToggle10Plus(CCObject*) {
    auto mod = Mod::get();
    mod->setSettingValue("show-10plus-only", !mod->getSettingValue<bool>("show-10plus-only"));
    if (auto overlay = FrameWindowOverlay::get()) overlay->refreshSettings();
}

void FWCSettingsPopup::onToggleMsHz(CCObject*) {
    auto mod = Mod::get();
    mod->setSettingValue("show-ms-hz", !mod->getSettingValue<bool>("show-ms-hz"));
    if (auto overlay = FrameWindowOverlay::get()) overlay->refreshSettings();
}

void FWCSettingsPopup::onToggleBucketMS(CCObject*) {
    auto mod = Mod::get();
    mod->setSettingValue("show-bucket-ms", !mod->getSettingValue<bool>("show-bucket-ms"));
    if (auto overlay = FrameWindowOverlay::get()) overlay->refreshSettings();
}

void FWCSettingsPopup::onToggleLastMS(CCObject*) {
    auto mod = Mod::get();
    mod->setSettingValue("show-last-ms", !mod->getSettingValue<bool>("show-last-ms"));
    if (auto overlay = FrameWindowOverlay::get()) overlay->refreshSettings();
}

void FWCSettingsPopup::onToggleCPS(CCObject*) {
    auto mod = Mod::get();
    mod->setSettingValue("show-cps", !mod->getSettingValue<bool>("show-cps"));
    if (auto overlay = FrameWindowOverlay::get()) overlay->refreshSettings();
}

void FWCSettingsPopup::onToggleGravityMini(CCObject*) {
    auto mod = Mod::get();
    mod->setSettingValue("show-gravity-mini", !mod->getSettingValue<bool>("show-gravity-mini"));
    if (auto overlay = FrameWindowOverlay::get()) overlay->refreshSettings();
}

void FWCSettingsPopup::onIncreaseFPS(CCObject*) {
    auto mod = Mod::get();
    int fps = static_cast<int>(mod->getSettingValue<int64_t>("analysis-fps"));
    if (fps < 360) fps += 60;
    mod->setSettingValue("analysis-fps", static_cast<int64_t>(fps));
    m_fpsLabel->setString(fmt::format("{}Hz", fps).c_str());
    FrameWindowAnalyzer::get()->setAnalysisFPS(fps);
    if (auto overlay = FrameWindowOverlay::get()) overlay->refreshSettings();
}

void FWCSettingsPopup::onDecreaseFPS(CCObject*) {
    auto mod = Mod::get();
    int fps = static_cast<int>(mod->getSettingValue<int64_t>("analysis-fps"));
    if (fps > 60) fps -= 60;
    mod->setSettingValue("analysis-fps", static_cast<int64_t>(fps));
    m_fpsLabel->setString(fmt::format("{}Hz", fps).c_str());
    FrameWindowAnalyzer::get()->setAnalysisFPS(fps);
    if (auto overlay = FrameWindowOverlay::get()) overlay->refreshSettings();
}

void FWCSettingsPopup::onPickMacroFile(CCObject*) {
    file::FilePickOptions options;
    options.filters.push_back({
        "Geometry Dash Replays & Macros",
        { "*.gdr2", "*.gdr", "*.mhr", "*.mhr.json", "*.json", "*.txt", "*.csv", "*.rply" }
    });

    async::spawn(
        file::pick(file::PickMode::OpenFile, options),
        [this](file::PickResult result) {
            if (result.isOk()) {
                auto opt = result.unwrap();
                if (opt.has_value()) {
                    std::string pickedPath = opt.value().string();
                    this->loadMacroFile(pickedPath);
                }
            }
        }
    );
}

void FWCSettingsPopup::onScanMacroFolder(CCObject*) {
    m_macroFiles.clear();
    auto dirs = MacroParser::getDefaultMacroDirs();

    for (auto const& d : dirs) {
        auto files = MacroParser::findMacroFiles(d);
        for (auto const& f : files) {
            m_macroFiles.push_back(f);
        }
    }

    if (m_macroFiles.empty()) {
        FLAlertLayer::create(
            "No Macro Files Found",
            "No macro files (.gdr2, .mhr, .json) found in default paths.\n\n"
            "Click 'Select File...' to browse your device storage, or place files in:\n"
            "- GD save folder/macros/\n"
            "- GD save folder/replays/\n"
#ifdef GEODE_IS_ANDROID
            "- /sdcard/GeometryDash/macros/\n"
            "- /storage/emulated/0/Download/\n"
#endif
            "\nSupported: Eclipse (.gdr2), MegaHack (.mhr), xdBot, xBot, TASBot, JSON",
            "OK"
        )->show();
        return;
    }

    // Load the first found macro
    this->loadMacroFile(m_macroFiles[0]);
}

void FWCSettingsPopup::loadMacroFile(std::string const& path) {
    auto data = MacroParser::parse(path);
    if (data.valid && !data.inputs.empty()) {
        auto analyzer = FrameWindowAnalyzer::get();
        analyzer->analyzeMacro(data);

        if (auto overlay = FrameWindowOverlay::get()) {
            overlay->updateDisplay();
            overlay->updatePosition();
        }

        std::string filename = std::filesystem::path(path).filename().string();
        m_macroInfoLabel->setString(
            fmt::format("Loaded: {} ({} clicks, {:.0f} FPS)",
                filename,
                data.inputs.size(),
                data.recordFPS
            ).c_str()
        );
        m_macroInfoLabel->setColor({ 0, 255, 120 });

        FLAlertLayer::create(
            "Macro Loaded!",
            fmt::format(
                "File: {}\n"
                "Format: {}\n"
                "Inputs: {}\n"
                "FPS: {:.0f}\n"
                "CBF (Frame Perfect): {}\n"
                "Average Window: {:.2f}f",
                filename,
                data.botName,
                data.inputs.size(),
                data.recordFPS,
                analyzer->getCBFCount(),
                analyzer->getAverageWindow()
            ).c_str(),
            "OK"
        )->show();
    } else {
        FLAlertLayer::create(
            "Macro Load Error",
            fmt::format("Failed to parse macro file:\n{}\n\nDetails: {}\n\nSupported: Eclipse (.gdr2), MegaHack (.mhr), xdBot, xBot, TASBot, JSON",
                std::filesystem::path(path).filename().string(),
                data.error.empty() ? "No valid click actions found" : data.error
            ).c_str(),
            "OK"
        )->show();
    }
}

void FWCSettingsPopup::onClearMacro(CCObject*) {
    auto analyzer = FrameWindowAnalyzer::get();
    analyzer->reset();
    if (auto overlay = FrameWindowOverlay::get()) {
        overlay->updateDisplay();
        overlay->updatePosition();
    }
    m_macroInfoLabel->setString("No macro loaded");
    m_macroInfoLabel->setColor({ 180, 180, 180 });
}

void FWCSettingsPopup::onResetData(CCObject*) {
    auto analyzer = FrameWindowAnalyzer::get();
    analyzer->reset();
    analyzer->setTracking(true);
    if (auto overlay = FrameWindowOverlay::get()) {
        overlay->updateDisplay();
        overlay->updatePosition();
    }
    m_macroInfoLabel->setString("Stats reset");
    m_macroInfoLabel->setColor({ 255, 255, 0 });
}
