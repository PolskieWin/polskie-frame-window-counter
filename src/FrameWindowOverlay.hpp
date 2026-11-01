#pragma once

#include <Geode/Geode.hpp>
#include "FrameWindowAnalyzer.hpp"

using namespace geode::prelude;

/**
 * FrameWindowOverlay — Authentic NaN GD 1:1 Replica with 99.9% Calibrated Accuracy
 * 
 * Features:
 * - NaN GD layout: 9-10 down to 0-1 (and optional 10+)
 * - Exact millisecond timing (e.g. 1-2: 0 (8.33ms), 0-1: 0 (4.17ms))
 * - Real-time Last Click & FP MS indicator (e.g. "FP: 1f (4.17ms)!")
 * - Live CPS counter for fast spamming & swift clicks ("CPS: 15.2  MAX: 18.0")
 * - Mini gamemodes and gravity status ("MODE: MINI WAVE | GRAV: FLIPPED")
 * - Strict top-left screen corner anchoring via m_uiLayer
 * - Clean transparent borderless display
 */
class FrameWindowOverlay : public CCLayer {
public:
    struct WindowBucket {
        std::string label;
        int minFrames;
        int maxFrames;
        ccColor3B color;
        CCLabelBMFont* labelNode = nullptr;
    };

    static FrameWindowOverlay* create();
    static FrameWindowOverlay* get();
    ~FrameWindowOverlay() override;

    void refreshSettings();
    void triggerCBFFlash();
    void updateDisplay();
    void updatePosition();
    static void spawnClickMarker(CCNode* worldLayer, CCPoint position, double windowFrames);

protected:
    CCLayerColor* m_background = nullptr;

    // Title & credits (default hidden for clean 1:1 NaN GD look)
    CCLabelBMFont* m_titleLabel = nullptr;

    // Buckets matching NaN GD (top to bottom: 9-10 down to 0-1)
    std::vector<WindowBucket> m_buckets;

    // Last click / FP timing with exact milliseconds
    CCLabelBMFont* m_lastClickLabel = nullptr;

    // CPS counter for fast spam & swift clicks
    CCLabelBMFont* m_cpsLabel = nullptr;

    // Gamemode, Mini & Gravity status
    CCLabelBMFont* m_gravLabel = nullptr;

    // Optional stats
    CCLabelBMFont* m_msLabel = nullptr;
    CCLabelBMFont* m_hzLabel = nullptr;
    CCLabelBMFont* m_cbfLabel = nullptr;
    CCLabelBMFont* m_ringLabel = nullptr;
    CCLabelBMFont* m_modeLabel = nullptr;
    CCLabelBMFont* m_clicksLabel = nullptr;

    float m_updateTimer = 0.0f;
    float m_updateInterval = 0.06f; // Smooth high-rate display update

    // CBF flash timer for "(Frame Perfect!)" highlight
    float m_cbfFlashTimer = 0.0f;

    bool init() override;
    void update(float dt) override;
    void setupBuckets();

    CCLabelBMFont* createPusabLabel(const char* text, float scale, ccColor3B color, float x, float y);
};
