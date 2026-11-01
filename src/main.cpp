/**
 * Polskie Frame Window Counter v3.1 for Geometry Dash (Android & Windows)
 * Made by Polskie
 * 
 * Features:
 * - NaN GD 1:1 replica in Pusab font (bigFont.fnt)
 * - Screen position pinned strictly to TOP-LEFT screen corner
 * - NaN GD frame window buckets: 9-10, 7-8, 5-6, 3-4, 1-2, 0-1 (and optional 10+)
 * - CBF count with "CBF: X (Frame Perfect!)" dynamic status
 * - Ring / Orb Frame Perfect detection & sounds
 * - Macro calculation & live playback input tracking (Works with Bots!)
 * - Replay event import: .gdr, .slc, .tcm, .ybot, and generic JSON streams
 * - Full RobTop-style UI settings popup in 100% English
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

static void recordFWCInput(CCNode* objectLayer, PlayerObject* player, bool isPress, int playerNum) {
    if (!player) return;
    auto analyzer = FrameWindowAnalyzer::get();
    auto clickCount = analyzer->getClicks().size();
    analyzer->onPlayerInput(isPress, player, playerNum);
    if (analyzer->getClicks().size() == clickCount) return;

    auto worldPosition = player->convertToWorldSpace({ 0.0f, 0.0f });
    auto markerPosition = objectLayer ? objectLayer->convertToNodeSpace(worldPosition) : worldPosition;
    FrameWindowOverlay::spawnClickMarker(
        objectLayer,
        markerPosition,
        analyzer->getLastClickWindowFrames(),
        analyzer->hasLastClickWindowMeasurement()
    );
    if (auto overlay = FrameWindowOverlay::get()) overlay->updateDisplay();
}

// ============================================================
// PlayLayer hooks — Lifecycle, UI layer attachment & updates
// ============================================================
class $modify(FWCPlayLayer, PlayLayer) {
    struct Fields {
        bool m_p1WasHolding = false;
        bool m_p2WasHolding = false;
    };

    bool init(GJGameLevel* level, bool dontCreateObjects) {
        if (!PlayLayer::init(level, dontCreateObjects)) {
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
            if (this->m_uiLayer) {
                this->m_uiLayer->addChild(g_overlay, 99999);
            } else {
                this->addChild(g_overlay, 99999);
            }
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

        if (player) {
            bool holdingP1 = player->m_holdingButtons[1] || player->m_jumpBuffered;
            if (holdingP1 && !m_fields->m_p1WasHolding) {
                if (!analyzer->hasRecordedThisFrame(1, true)) {
                    recordFWCInput(this->m_objectLayer, player, true, 1);
                    if (analyzer->isLastClickCBF() && g_overlay) {
                        g_overlay->triggerCBFFlash();
                    }
                }
            } else if (!holdingP1 && m_fields->m_p1WasHolding) {
                if (!analyzer->hasRecordedThisFrame(1, false)) {
                    recordFWCInput(this->m_objectLayer, player, false, 1);
                }
            }
            m_fields->m_p1WasHolding = holdingP1;
        }

        PlayerObject* p2 = this->m_player2;
        if (p2) {
            bool holdingP2 = p2->m_holdingButtons[1] || p2->m_jumpBuffered;
            if (holdingP2 && !m_fields->m_p2WasHolding) {
                if (!analyzer->hasRecordedThisFrame(2, true)) {
                    recordFWCInput(this->m_objectLayer, p2, true, 2);
                    if (analyzer->isLastClickCBF() && g_overlay) {
                        g_overlay->triggerCBFFlash();
                    }
                }
            } else if (!holdingP2 && m_fields->m_p2WasHolding) {
                if (!analyzer->hasRecordedThisFrame(2, false)) {
                    recordFWCInput(this->m_objectLayer, p2, false, 2);
                }
            }
            m_fields->m_p2WasHolding = holdingP2;
        }
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

        log::info("[Polskie FWC] Level complete! Clicks: {}, CBF: {}, Ring FP: {}",
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

#ifndef GEODE_IS_ANDROID
class $modify(FWCPlayerObject, PlayerObject) {
    void ringJump(RingObject* ring, bool skipCheck) {
        PlayerObject::ringJump(ring, skipCheck);

        auto mod = Mod::get();
        if (!mod->getSettingValue<bool>("enabled")) return;
        if (!mod->getSettingValue<bool>("show-ring-fp")) return;

        auto analyzer = FrameWindowAnalyzer::get();
        if (analyzer->isTracking()) {
            analyzer->onRingJump(this, ring);
        }
    }

    bool pushButton(PlayerButton button) {
        bool res = PlayerObject::pushButton(button);

        auto mod = Mod::get();
        if (mod->getSettingValue<bool>("enabled")) {
            auto analyzer = FrameWindowAnalyzer::get();
            if (analyzer->isTracking()) {
                auto pl = PlayLayer::get();
                int playerNum = (pl && this == pl->m_player2) ? 2 : 1;
                auto playLayer = PlayLayer::get();
                recordFWCInput(playLayer ? playLayer->m_objectLayer : nullptr, this, true, playerNum);
                if (analyzer->isLastClickCBF() && g_overlay) {
                    g_overlay->triggerCBFFlash();
                }
            }
        }
        return res;
    }

    bool releaseButton(PlayerButton button) {
        bool res = PlayerObject::releaseButton(button);

        auto mod = Mod::get();
        if (mod->getSettingValue<bool>("enabled")) {
            auto analyzer = FrameWindowAnalyzer::get();
            if (analyzer->isTracking()) {
                auto pl = PlayLayer::get();
                int playerNum = (pl && this == pl->m_player2) ? 2 : 1;
                auto playLayer = PlayLayer::get();
                recordFWCInput(playLayer ? playLayer->m_objectLayer : nullptr, this, false, playerNum);
            }
        }
        return res;
    }
};
#endif

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
            recordFWCInput(this->m_objectLayer, player, down, playerNum);

            if (analyzer->isLastClickCBF() && g_overlay) {
                g_overlay->triggerCBFFlash();
            }
        }
    }
};

class $modify(FWCPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        auto mod = Mod::get();
        if (!mod->getSettingValue<bool>("enabled")) return;

        auto winSize = CCDirector::get()->getWinSize();

        auto menu = CCMenu::create();
        menu->setPosition({ 0.f, 0.f });
        this->addChild(menu, 100);

        auto btnSpr = ButtonSprite::create("FWC", "bigFont.fnt", "GJ_button_01.png", 0.75f);
        btnSpr->setScale(0.70f);

        auto btn = CCMenuItemSpriteExtra::create(
            btnSpr, this,
            menu_selector(FWCPauseLayer::onFWCSettings)
        );

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

$on_mod(Loaded) {
    log::info("[Polskie Frame Window Counter] Loaded successfully! Author: Polskie");
    log::info("[Polskie FWC] Supports JSON event streams: .gdr, .slc, .tcm, .ybot, and generic JSON");
}
