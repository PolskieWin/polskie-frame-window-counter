#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include "FrameWindowAnalyzer.hpp"
#include "MacroParser.hpp"

using namespace geode::prelude;

/**
 * FWCSettingsPopup — RobTop style GUI with Pusab font
 * Made by Polskie
 */
class FWCSettingsPopup : public Popup {
protected:
    CCMenu* m_mainMenu = nullptr;
    CCLabelBMFont* m_statusLabel = nullptr;
    CCLabelBMFont* m_macroInfoLabel = nullptr;
    CCLabelBMFont* m_fpsLabel = nullptr;

    CCMenuItemToggler* m_enabledToggle = nullptr;
    CCMenuItemToggler* m_soundToggle = nullptr;
    CCMenuItemToggler* m_cbfToggle = nullptr;
    CCMenuItemToggler* m_ringToggle = nullptr;
    CCMenuItemToggler* m_higherToggle = nullptr;
    CCMenuItemToggler* m_10plusToggle = nullptr;
    CCMenuItemToggler* m_msHzToggle = nullptr;

    std::vector<std::string> m_macroFiles;

    bool init(float width, float height);

    void onToggleEnabled(CCObject* sender);
    void onToggleSound(CCObject* sender);
    void onToggleCBF(CCObject* sender);
    void onToggleRing(CCObject* sender);
    void onToggleHigher(CCObject* sender);
    void onToggle10Plus(CCObject* sender);
    void onToggleMsHz(CCObject* sender);

    void onIncreaseFPS(CCObject* sender);
    void onDecreaseFPS(CCObject* sender);

    void onPickMacroFile(CCObject* sender);
    void onScanMacroFolder(CCObject* sender);
    void onClearMacro(CCObject* sender);
    void onResetData(CCObject* sender);

    void loadMacroFile(std::string const& path);

    CCMenuItemToggler* createRobTopToggle(std::string const& label, bool defaultVal,
                                          SEL_MenuHandler callback, float x, float y);
    void updateStatusLabel();

public:
    static FWCSettingsPopup* create();
};
