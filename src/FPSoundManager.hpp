#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

/**
 * FPSoundManager
 * 
 * Handles frame-perfect sound effects like NaN GD does.
 * When a click lands on a 1-frame window (frame perfect / CBF),
 * plays a satisfying sound effect.
 * 
 * Supports multiple sound types:
 * - ding: High-pitched ding (like a bell)
 * - chime: Soft chime
 * - click: Sharp click sound
 * - coin: GD coin collect sound
 */
class FPSoundManager {
public:
    static FPSoundManager* get();

    // Play the frame-perfect sound
    void playFPSound();

    // Play a sound for near-frame-perfect (2-3f window)
    void playNearFPSound();

    // Play ring frame-perfect sound
    void playRingFPSound();

    // Cooldown to prevent sound spam
    void update(float dt);

private:
    FPSoundManager() = default;
    static FPSoundManager* s_instance;

    float m_cooldown = 0.0f;
    float m_minInterval = 0.05f; // 50ms minimum between sounds
};

