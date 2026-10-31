/**
 * Polskie Frame Window Counter v3.0 for Geometry Dash (Android & Windows)
 * Made by Polskie
 * 
 * Features:
 * - NaN GD & Razy's Frame Windows layout with Pusab font (bigFont.fnt)
 * - Frame window buckets: 0-1, 1-2, 2-3, 3-4, 4-5, 5-6, 6-8, 8-10, 10+
 * - CBF count with "CBF: X (Frame Perfect!)" dynamic status
 * - Ring / Orb Frame Perfect detection (ringJump hook)
 * - Frame-perfect sounds (NaN GD style)
 * - Eclipse Menu (.gdr2) macro import ("esplice") + MegaHack (.mhr) + JSON
 * - Full RobTop-style UI settings popup
 * - Compatible with GD 2.2071, 2.2074, 2.2091
 */

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/PauseLayer.hpp>

#include "FrameWindowAnalyzer.hpp"
#include "FrameWindowOverlay.hpp"
#include "MacroParser.hpp"
#include "FWCSettingsPopup.hpp"
#include "FPSoundManager.hpp"

using namespace geode::prelude;

static FrameWindowOverlay* g_overlay = nullptr;

// ============================================================
// PlayLayer hooks — Lifecycle & updates
// ============================================================
class $modify(FWCPlayLayer, PlayLayer) {

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) {
            return false;
        }

        auto mod = Mod::get();
        if (!mod->getSettingValue<bool>("enabled")) return true;

        auto analyzer = FrameWindowAnalyzer::get();

        if (!analyzer->hasMacroLoaded() || !mod->getSettingValue<bool>("macro-analysis-mode")) {
            analyzer->reset();
            analyzer->setAnalysisFPS(
                static_cast<int>(mod->getSettingValue<int64_t>("analysis-fps"))
            );
            analyzer->setTracking(true);
        }

        if (g_overlay) {
            g_overlay->removeFromParent();
            g_overlay = nullptr;
        }

        g_overlay = FrameWindowOverlay::create();
        if (g_overlay) {
            this->addChild(g_overlay, 99999);
        }

        return true;
    }

    void update(float dt) {
        PlayLayer::update(dt);

        auto mod = Mod::get();
        if (!mod->getSettingValue<bool>("enabled")) return;

        auto analyzer = FrameWindowAnalyzer::get();
        if (analyzer->hasMacroLoaded() && mod->getSettingValue<bool>("macro-analysis-mode")) {
            return;
        }

        if (!analyzer->isTracking()) return;

        PlayerObject* player = this->m_player1;
        float levelLength = this->m_levelLength;
        analyzer->onUpdate(dt, player, levelLength);
    }

    void resetLevel() {
        PlayLayer::resetLevel();

        auto mod = Mod::get();
        if (!mod->getSettingValue<bool>("enabled")) return;

        auto analyzer = FrameWindowAnalyzer::get();
        if (analyzer->hasMacroLoaded() && mod->getSettingValue<bool>("macro-analysis-mode")) {
            return;
        }

        analyzer->reset();
        analyzer->setTracking(true);
    }

    void levelComplete() {
        PlayLayer::levelComplete();

        auto analyzer = FrameWindowAnalyzer::get();
        analyzer->setTracking(false);

        log::info("[Polskie FWC] Poziom ukończony! Kliknięć: {}, CBF: {}, Ring FP: {}",
            analyzer->getTotalClicks(),
            analyzer->getCBFCount(),
            analyzer->getRingFPCount()
        );
    }

    void onQuit() {
        auto analyzer = FrameWindowAnalyzer::get();
        analyzer->setTracking(false);

        if (!analyzer->hasMacroLoaded()) {
            analyzer->reset();
        }

        if (g_overlay) {
            g_overlay->removeFromParent();
            g_overlay = nullptr;
        }

        PlayLayer::onQuit();
    }
};

// ============================================================
// PlayerObject hook — Ring / Orb Frame Perfect detection
// ============================================================
class $modify(FWCPlayerObject, PlayerObject) {

    void ringJump(RingObject* ring, bool skipCheck) {
        PlayerObject::ringJump(ring, skipCheck);

        auto mod = Mod::get();
        if (!mod->getSettingValue<bool>("enabled")) return;
        if (!mod->getSettingValue<bool>("show-ring-fp")) return;

        // Register ring interaction in analyzer
        auto analyzer = FrameWindowAnalyzer::get();
        if (analyzer->isTracking()) {
            analyzer->onRingJump(this, ring);
        }
    }
};

// ============================================================
// GJBaseGameLayer hooks — Live Input Tracking (Works with Bots!)
// ============================================================
class $modify(FWCGameLayer, GJBaseGameLayer) {

    void handleButton(bool down, int button, bool isPlayer1) {
        GJBaseGameLayer::handleButton(down, button, isPlayer1);

        auto mod = Mod::get();
        if (!mod->getSettingValue<bool>("enabled")) return;

        auto analyzer = FrameWindowAnalyzer::get();
        if (analyzer->hasMacroLoaded() && mod->getSettingValue<bool>("macro-analysis-mode")) {
            return;
        }

        if (!analyzer->isTracking()) return;

        int playerNum = isPlayer1 ? 1 : 2;
        PlayerObject* player = isPlayer1 ? this->m_player1 : this->m_player2;

        if (player) {
            analyzer->onPlayerInput(down, player, playerNum);

            // If it was frame-perfect, trigger flash in overlay
            if (analyzer->isLastClickCBF() && g_overlay) {
                g_overlay->triggerCBFFlash();
            }
        }
    }
};

// ============================================================
// PauseLayer hook — RobTop style Pusab button
// ============================================================
class $modify(FWCPauseLayer, PauseLayer) {

    void customSetup() {
        PauseLayer::customSetup();

        auto mod = Mod::get();
        if (!mod->getSettingValue<bool>("enabled")) return;

        auto winSize = CCDirector::get()->getWinSize();

        auto menu = CCMenu::create();
        menu->setPosition({ 0.f, 0.f });
        this->addChild(menu, 100);

        // RobTop style button with Pusab font
        auto btnSpr = ButtonSprite::create("FWC", "bigFont.fnt", "GJ_button_01.png", 0.75f);
        btnSpr->setScale(0.70f);

        auto btn = CCMenuItemSpriteExtra::create(
            btnSpr, this,
            menu_selector(FWCPauseLayer::onFWCSettings)
        );

        // Position in pause menu
        btn->setPosition({
            winSize.width - 45.f,
            winSize.height - 35.f
        });
        menu->addChild(btn);

        if (g_overlay) {
            g_overlay->refreshSettings();
        }
    }

    void onFWCSettings(CCObject* sender) {
        FWCSettingsPopup::create()->show();
    }
};

// ============================================================
// Mod entry point
// ============================================================
$on_mod(Loaded) {
    log::info("[Polskie Frame Window Counter v3] Załadowano pomyślnie! Autor: Polskie");
    log::info("[Polskie FWC] Obsługa: Eclipse Menu (.gdr2), MegaHack (.mhr), JSON");
}
