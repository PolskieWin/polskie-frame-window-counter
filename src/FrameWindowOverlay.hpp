#pragma once

#include <Geode/Geode.hpp>
#include "FrameWindowAnalyzer.hpp"

using namespace geode::prelude;

/**
 * FrameWindowOverlay — Authentic NaN GD 1:1 Replica
 * 
 * Authentic RobTop HUD with Pusab font (bigFont.fnt):
 * - NaN GD layout:
 *     10+:  X (Violet)
 *     9-10: X (Purple)
 *     7-8:  X (Light Blue)
 *     5-6:  X (Lime Green)
 *     3-4:  X (Yellow)
 *     1-2:  X (Orange)
 *     0-1:  X (Frame Perfect Pink/Red)
 * - Optional MS & Hz counters (Razy / NaN GD style)
 * - Optional CBF status: "CBF: X (Frame Perfect!)"
 * - Optional Ring FP counter: "RINGS FP: X/Y"
 * - Optional Gamemode & total clicks
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
    void refreshSettings();
    void triggerCBFFlash();

protected:
    CCLayerColor* m_background = nullptr;

    // Title & credits (Pusab font)
    CCLabelBMFont* m_titleLabel = nullptr;

    // Buckets matching NaN GD
    std::vector<WindowBucket> m_buckets;

    // MS and Hz stats
    CCLabelBMFont* m_msLabel = nullptr;
    CCLabelBMFont* m_hzLabel = nullptr;

    // CBF (Click Before Frame = Frame Perfect)
    CCLabelBMFont* m_cbfLabel = nullptr;

    // Ring / Orb FP
    CCLabelBMFont* m_ringLabel = nullptr;

    // Gamemode & Clicks
    CCLabelBMFont* m_modeLabel = nullptr;
    CCLabelBMFont* m_clicksLabel = nullptr;

    float m_updateTimer = 0.0f;
    float m_updateInterval = 0.10f;

    // CBF flash timer for "(Frame Perfect!)" highlight
    float m_cbfFlashTimer = 0.0f;

    bool init() override;
    void update(float dt) override;
    void updateDisplay();
    void updatePosition();
    void setupBuckets();

    CCLabelBMFont* createPusabLabel(const char* text, float scale, ccColor3B color, float x, float y);
};
