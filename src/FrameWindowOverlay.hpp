#pragma once

#include <Geode/Geode.hpp>
#include "FrameWindowAnalyzer.hpp"

using namespace geode::prelude;

/**
 * FrameWindowOverlay — Authentic NaN GD 1:1 Replica
 * 
 * Authentic RobTop HUD with Pusab font (bigFont.fnt):
 * - NaN GD layout:
 *     9-10: X (Purple #8A5FF9)
 *     7-8:  X (Cyan #59C9D7)
 *     5-6:  X (Mint Green #77DF91)
 *     3-4:  X (Yellow #D0D53B)
 *     1-2:  X (Orange #E8A020)
 *     0-1:  X (Frame Perfect Red/Pink #EF4F50)
 *     10+:  X (Violet #BA5DF9 - optional overflow bucket)
 * - Clean transparent borderless display (0 opacity default)
 * - Strict top-left screen corner anchoring via m_uiLayer
 * - Optional CBF status: "CBF: X (Frame Perfect!)"
 * - Optional Ring FP counter: "RINGS FP: X/Y"
 * - Optional MS & Hz counters (Razy / NaN GD style)
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
    static FrameWindowOverlay* get();
    ~FrameWindowOverlay() override;

    void refreshSettings();
    void triggerCBFFlash();
    void updateDisplay();
    void updatePosition();

protected:
    CCLayerColor* m_background = nullptr;

    // Title & credits (Pusab font, default hidden for 1:1 NaN GD look)
    CCLabelBMFont* m_titleLabel = nullptr;

    // Buckets matching NaN GD (top to bottom: 9-10 down to 0-1)
    std::vector<WindowBucket> m_buckets;

    // Optional stats
    CCLabelBMFont* m_msLabel = nullptr;
    CCLabelBMFont* m_hzLabel = nullptr;
    CCLabelBMFont* m_cbfLabel = nullptr;
    CCLabelBMFont* m_ringLabel = nullptr;
    CCLabelBMFont* m_modeLabel = nullptr;
    CCLabelBMFont* m_clicksLabel = nullptr;

    float m_updateTimer = 0.0f;
    float m_updateInterval = 0.08f;

    // CBF flash timer for "(Frame Perfect!)" highlight
    float m_cbfFlashTimer = 0.0f;

    bool init() override;
    void update(float dt) override;
    void setupBuckets();

    CCLabelBMFont* createPusabLabel(const char* text, float scale, ccColor3B color, float x, float y);
};
