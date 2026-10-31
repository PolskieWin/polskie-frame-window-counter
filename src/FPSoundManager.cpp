#include "FPSoundManager.hpp"

FPSoundManager* FPSoundManager::s_instance = nullptr;

FPSoundManager* FPSoundManager::get() {
    if (!s_instance) {
        s_instance = new FPSoundManager();
    }
    return s_instance;
}

void FPSoundManager::playFPSound() {
    if (m_cooldown > 0.0f) return;

    auto mod = Mod::get();
    if (!mod->getSettingValue<bool>("fp-sound-enabled")) return;

    std::string soundType = mod->getSettingValue<std::string>("fp-sound-type");

    // Use GD's built-in sounds for maximum compatibility on Android
    // These are sounds that exist in every GD installation
    std::string soundFile;

    if (soundType == "ding") {
        // Reward/achievement style ding
        soundFile = "reward01.ogg";
    } else if (soundType == "chime") {
        // Soft chime sound
        soundFile = "chime01.ogg";
    } else if (soundType == "click") {
        // Sharp click
        soundFile = "quitSound_01.ogg";
    } else if (soundType == "coin") {
        // GD coin collect — very recognizable
        soundFile = "gold01.ogg";
    } else {
        soundFile = "reward01.ogg";
    }

    // Try to play custom sound from mod resources first
    auto customPath = mod->getResourcesDir() / ("fp_" + soundType + ".ogg");
    if (std::filesystem::exists(customPath)) {
        FMODAudioEngine::sharedEngine()->playEffect(customPath.string());
    } else {
        // Fall back to GD's built-in sounds
        FMODAudioEngine::sharedEngine()->playEffect(soundFile);
    }

    m_cooldown = m_minInterval;
}

void FPSoundManager::playNearFPSound() {
    if (m_cooldown > 0.0f) return;

    auto mod = Mod::get();
    if (!mod->getSettingValue<bool>("fp-sound-enabled")) return;

    // Softer sound for near-frame-perfect (2-3f window)
    // Use a quieter built-in sound
    FMODAudioEngine::sharedEngine()->playEffect("counter003.ogg");

    m_cooldown = m_minInterval;
}

void FPSoundManager::playRingFPSound() {
    if (m_cooldown > 0.0f) return;

    auto mod = Mod::get();
    if (!mod->getSettingValue<bool>("fp-sound-enabled")) return;
    if (!mod->getSettingValue<bool>("show-ring-fp")) return;

    // Special sound for ring frame-perfect hits
    auto customPath = mod->getResourcesDir() / "fp_ring.ogg";
    if (std::filesystem::exists(customPath)) {
        FMODAudioEngine::sharedEngine()->playEffect(customPath.string());
    } else {
        FMODAudioEngine::sharedEngine()->playEffect("gold02.ogg");
    }

    m_cooldown = m_minInterval;
}

void FPSoundManager::update(float dt) {
    if (m_cooldown > 0.0f) {
        m_cooldown -= dt;
        if (m_cooldown < 0.0f) m_cooldown = 0.0f;
    }
}

