#pragma once

#include <Geode/Geode.hpp>
#include "FrameWindowAnalyzer.hpp"

using namespace geode::prelude;

/**
 * FrameWindowOverlay — Polskie Edition
 * 
 * Authentic RobTop-style HUD with Pusab font (bigFont.fnt):
 * - NaN GD & Razy's Frame Windows layout:
 *     10+:  X (Purple)
 *     8-10: X (Blue)
 *     6-8:  X (Light Blue)
 *     5-6:  X (Cyan)
 *     4-5:  X (Green)
 *     3-4:  X (Yellow)
 *     2-3:  X (Orange)
 *     1-2:  X (Red)
 *     0-1:  X (Pink/Magenta - Frame Perfect / CBF!)
 *     XX.XXMS
 *     XXHz
 * - CBF status: "CBF: X (Frame Perfect!)" when active, "CBF: X" normally
 * - Ring FP counter: "Rings FP: X/Y"
 * - Mode indicator: "Mode: Cube / Wave / etc."
 * - Option to enable/disable higher numbers (8-10, 10+)
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

    // Buckets matching Razy's Frame Windows
    std::vector<WindowBucket> m_buckets;

    // MS and Hz stats (from video)
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
    float m_updateInterval = 0.12f;

    // CBF flash timer for the "(Frame Perfect!)" highlight
    float m_cbfFlashTimer = 0.0f;

    bool init() override;
    void update(float dt) override;
    void updateDisplay();
    void updatePosition();
    void setupBuckets();

    CCLabelBMFont* createPusabLabel(const char* text, float scale, ccColor3B color, float x, float y);
};
