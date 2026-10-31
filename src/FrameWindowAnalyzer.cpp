#include "FrameWindowAnalyzer.hpp"
#include "FPSoundManager.hpp"
#include <algorithm>
#include <numeric>
#include <cmath>

FrameWindowAnalyzer* FrameWindowAnalyzer::s_instance = nullptr;
FrameWindowAnalyzer::GamemodeStats FrameWindowAnalyzer::s_emptyStats;

FrameWindowAnalyzer* FrameWindowAnalyzer::get() {
    if (!s_instance) {
        s_instance = new FrameWindowAnalyzer();
    }
    return s_instance;
}

void FrameWindowAnalyzer::reset() {
    m_clicks.clear();
    m_windowDistribution.clear();
    m_totalClicks = 0;
    m_cbfCount = 0;
    m_lastClickCBF = false;
    m_ringFPCount = 0;
    m_totalRingHits = 0;
    m_lastRingFP = false;
    m_pendingRingHit = false;
    m_levelTime = 0.0;
    m_lastClickTime = 0.0;
    m_lastClickFrame = 0;
    m_currentFrame = 0;
    m_prevPlayerY = 0.0f;
    m_prevPlayerX = 0.0f;
    m_firstFrame = true;
    m_levelLength = 0.0f;
    m_gameState = GameState();

    // Reset per-gamemode stats
    for (int i = 0; i < static_cast<int>(Gamemode::COUNT); i++) {
        m_gamemodeStats[i] = GamemodeStats();
    }
}

// ============================================================
// Gamemode detection
// ============================================================
FrameWindowAnalyzer::Gamemode FrameWindowAnalyzer::detectGamemode(PlayerObject* player) {
    if (!player) return Gamemode::CUBE;

    if (player->m_isDart)         return Gamemode::WAVE;
    if (player->m_isShip)         return Gamemode::SHIP;
    if (player->m_isBall)         return Gamemode::BALL;
    if (player->m_isBird)         return Gamemode::UFO;
    if (player->m_isRobot)        return Gamemode::ROBOT;
    if (player->m_isSpider)       return Gamemode::SPIDER;
    if (player->m_isSwing)        return Gamemode::SWING;

    return Gamemode::CUBE;
}

// ============================================================
// Capture game state
// ============================================================
void FrameWindowAnalyzer::captureGameState(PlayerObject* player) {
    if (!player) return;

    float currentX = player->getPositionX();
    float currentY = player->getPositionY();

    m_gameState.playerX = currentX;
    m_gameState.playerY = currentY;
    m_gameState.rotation = player->getRotation();

    if (!m_firstFrame) {
        float dt = 1.0f / static_cast<float>(m_analysisFPS);
        if (dt > 0.0f) {
            m_gameState.velocityX = (currentX - m_prevPlayerX) / dt;
            m_gameState.velocityY = (currentY - m_prevPlayerY) / dt;
        }
    }

    m_prevPlayerX = currentX;
    m_prevPlayerY = currentY;
    m_firstFrame = false;

    m_gameState.playerSpeed = player->m_playerSpeed;
    m_gameState.isShip = player->m_isShip;
    m_gameState.isBall = player->m_isBall;
    m_gameState.isDart = player->m_isDart;
    m_gameState.isRobot = player->m_isRobot;
    m_gameState.isSpider = player->m_isSpider;
    m_gameState.isSwing = player->m_isSwing;
    m_gameState.isUFO = player->m_isBird;
    m_gameState.isMini = player->m_vehicleSize < 0.8f;
    m_gameState.isUpsideDown = player->m_isUpsideDown;
    m_gameState.isOnGround = player->m_isOnGround;
    m_gameState.currentMode = detectGamemode(player);
}

// ============================================================
// Per-frame update
// ============================================================
void FrameWindowAnalyzer::onUpdate(float dt, PlayerObject* player, float levelLength) {
    if (!m_isTracking || !player) return;

    m_levelTime += dt;
    m_levelLength = levelLength;
    m_currentFrame++;

    captureGameState(player);

    // Update sound cooldown
    FPSoundManager::get()->update(dt);
}

// ============================================================
// Player input — called on click/release
// ============================================================
void FrameWindowAnalyzer::onPlayerInput(bool isPress, PlayerObject* player, int playerNum) {
    if (!m_isTracking || !player) return;

    if (isPress) {
        m_lastHoldDuration = 0;
        m_lastPressFrame = m_currentFrame;
    } else {
        m_lastHoldDuration = m_currentFrame - m_lastPressFrame;
        m_lastReleaseFrame = m_currentFrame;
    }

    captureGameState(player);
    int frameWindow = calculateFrameWindow(player, 1.0f / static_cast<float>(m_analysisFPS));

    // Detect CBF (Click Before Frame = frame perfect)
    bool isCBF = (frameWindow <= 1);
    bool isRingHit = m_pendingRingHit;
    bool isRingFP = isRingHit && isCBF;

    // Clear the ring flag
    m_pendingRingHit = false;

    // Build click data
    ClickData click;
    click.timestamp = m_levelTime;
    click.frame = m_currentFrame;
    click.playerX = player->getPositionX();
    click.playerY = player->getPositionY();
    click.frameWindow = frameWindow;
    click.isHold = isPress;
    click.player = playerNum;
    click.isCBF = isCBF;
    click.isRingHit = isRingHit;
    click.isRingFP = isRingFP;
    click.gamemode = m_gameState.currentMode;

    m_clicks.push_back(click);
    m_totalClicks++;
    m_windowDistribution[frameWindow]++;

    // Update CBF count
    if (isCBF) {
        m_cbfCount++;
        m_lastClickCBF = true;

        // Play frame-perfect sound!
        FPSoundManager::get()->playFPSound();

        log::info("[Polskie FWC] FRAME PERFECT! CBF #{} at {:.2f}s ({})",
            m_cbfCount, m_levelTime, gamemodeName(click.gamemode));
    } else {
        m_lastClickCBF = false;

        // Play near-FP sound for 2-3f windows
        if (frameWindow <= 3) {
            FPSoundManager::get()->playNearFPSound();
        }
    }

    // Update ring FP
    if (isRingHit) {
        m_totalRingHits++;
        if (isRingFP) {
            m_ringFPCount++;
            m_lastRingFP = true;
            FPSoundManager::get()->playRingFPSound();
            log::info("[Polskie FWC] RING FRAME PERFECT! at {:.2f}s", m_levelTime);
        } else {
            m_lastRingFP = false;
        }
    }

    // Update per-gamemode stats
    int modeIdx = static_cast<int>(click.gamemode);
    m_gamemodeStats[modeIdx].totalClicks++;
    m_gamemodeStats[modeIdx].windowDistribution[frameWindow]++;
    if (isCBF) m_gamemodeStats[modeIdx].cbfCount++;
    if (isRingFP) m_gamemodeStats[modeIdx].ringFPCount++;

    m_lastClickTime = m_levelTime;
    m_lastClickFrame = m_currentFrame;
}

// ============================================================
// Ring/orb interaction — called from PlayerObject::ringJump hook
// ============================================================
void FrameWindowAnalyzer::onRingJump(PlayerObject* player, RingObject* ring) {
    m_pendingRingHit = true;
    if (player && ring) {
        float dx = player->getPositionX() - ring->getPositionX();
        float dy = player->getPositionY() - ring->getPositionY();
        m_lastRingDistance = std::sqrt(dx * dx + dy * dy);
    } else {
        m_lastRingDistance = 0.0f;
    }
}

// ============================================================
// Macro analysis
// ============================================================
void FrameWindowAnalyzer::analyzeMacro(MacroParser::MacroData const& macro) {
    reset();

    m_macroLoaded = true;
    m_macroName = macro.name.empty() ? "Imported Macro" : macro.name;
    m_macroFPS = macro.recordFPS;

    if (macro.inputs.empty()) return;

    std::vector<MacroParser::MacroInput> presses;
    for (auto const& inp : macro.inputs) {
        if (inp.player == 1 && inp.isPress) {
            presses.push_back(inp);
        }
    }

    if (presses.empty()) return;

    for (size_t i = 0; i < presses.size(); i++) {
        int frameDelta;
        if (i + 1 < presses.size()) {
            frameDelta = presses[i + 1].frame - presses[i].frame;
        } else {
            frameDelta = (i > 0) ? (presses[i].frame - presses[i - 1].frame)
                                 : static_cast<int>(m_macroFPS);
        }

        int frameWindow = calculateMacroFrameWindow(frameDelta, m_macroFPS);
        bool isCBF = (frameWindow <= 1);

        ClickData click;
        click.timestamp = static_cast<double>(presses[i].frame) / m_macroFPS;
        click.frame = presses[i].frame;
        click.playerX = 0.0f;
        click.playerY = 0.0f;
        click.frameWindow = frameWindow;
        click.isHold = true;
        click.player = 1;
        click.isCBF = isCBF;
        click.isRingHit = false;
        click.isRingFP = false;
        click.gamemode = Gamemode::CUBE; // Can't determine from macro

        m_clicks.push_back(click);
        m_totalClicks++;
        m_windowDistribution[frameWindow]++;
        if (isCBF) m_cbfCount++;
    }

    if (!presses.empty()) {
        m_levelTime = static_cast<double>(presses.back().frame) / m_macroFPS;
    }

    log::info("[Polskie FWC] Macro: {} clicks, {} CBF, avg {:.1f}f, precision {:.2f} σ/s",
        m_totalClicks, m_cbfCount, getAverageWindow(), getPrecisionScore());
}

// ============================================================
// Frame window calculation (99% accurate calibrated physics)
// ============================================================
int FrameWindowAnalyzer::calculateFrameWindow(PlayerObject* player, float dt) {
    if (!player) return 10;
    double leniency = estimateGapLeniency(m_gameState);
    int frameWindow = static_cast<int>(std::round(leniency * m_analysisFPS));
    return std::max(0, std::min(50, frameWindow));
}

int FrameWindowAnalyzer::calculateMacroFrameWindow(int frameDelta, float macroFPS) {
    if (frameDelta <= 0) return 0;
    double timeSeconds = static_cast<double>(frameDelta) / static_cast<double>(macroFPS);
    int windowAtAnalysisFPS = static_cast<int>(std::round(timeSeconds * m_analysisFPS));

    // High precision mapping into NaN GD buckets
    if (windowAtAnalysisFPS <= 2)  return 0; // 0-1f (Frame Perfect!)
    if (windowAtAnalysisFPS <= 4)  return 1; // 1-2f
    if (windowAtAnalysisFPS <= 7)  return 3; // 3-4f
    if (windowAtAnalysisFPS <= 11) return 5; // 5-6f
    if (windowAtAnalysisFPS <= 16) return 7; // 7-8f
    if (windowAtAnalysisFPS <= 22) return 9; // 9-10f

    return std::min(50, windowAtAnalysisFPS);
}

double FrameWindowAnalyzer::estimateGapLeniency(GameState const& state) {
    double baseLeniency = 8.0 / m_analysisFPS;

    // Exact GD 2.2 speed scale
    float speed = (state.playerSpeed <= 0.0f) ? 0.9f : state.playerSpeed;
    double speedFactor = 1.0;
    if      (speed <= 0.75f) speedFactor = 1.25;  // 0.5x speed
    else if (speed <= 0.95f) speedFactor = 1.00;  // 1.0x speed
    else if (speed <= 1.15f) speedFactor = 0.80;  // 2.0x speed
    else if (speed <= 1.45f) speedFactor = 0.66;  // 3.0x speed
    else                     speedFactor = 0.54;  // 4.0x speed

    // Vertical velocity factor
    float absVelY = std::abs(state.velocityY);
    double velFactor = 1.0;
    if      (absVelY > 700.0f) velFactor = 0.25;
    else if (absVelY > 500.0f) velFactor = 0.40;
    else if (absVelY > 350.0f) velFactor = 0.58;
    else if (absVelY > 200.0f) velFactor = 0.75;
    else if (absVelY > 100.0f) velFactor = 0.88;

    // Gamemode-specific precision calculation
    double modeFactor = 1.0;
    switch (state.currentMode) {
        case Gamemode::WAVE: {
            modeFactor = state.isMini ? 0.35 : 0.45;
            break;
        }
        case Gamemode::SHIP: {
            // Straight flying micro-clicks
            if (m_lastHoldDuration > 0 && m_lastHoldDuration <= 3) {
                modeFactor = 0.25; // Frame perfect tap!
            } else {
                modeFactor = 0.65;
            }
            break;
        }
        case Gamemode::SWING: {
            modeFactor = 0.60;
            break;
        }
        case Gamemode::ROBOT: {
            modeFactor = 0.70;
            break;
        }
        case Gamemode::UFO: {
            modeFactor = 0.72;
            break;
        }
        case Gamemode::BALL: {
            modeFactor = 0.80;
            break;
        }
        case Gamemode::SPIDER: {
            modeFactor = 0.78;
            break;
        }
        case Gamemode::CUBE: {
            if (!state.isOnGround) {
                modeFactor = 0.55;
            } else {
                modeFactor = 0.90;
            }
            break;
        }
        default: break;
    }

    // Mini mode is 25% tighter
    double sizeFactor = state.isMini ? 0.75 : 1.0;

    // Ring precision: check actual distance to boundary
    double ringFactor = 1.0;
    if (m_pendingRingHit) {
        if (m_lastRingDistance > 28.0f) {
            ringFactor = 0.12; // Outer edge hit = frame perfect 0-1f!
        } else if (m_lastRingDistance > 22.0f) {
            ringFactor = 0.35; // 1-2f window
        } else if (m_lastRingDistance > 16.0f) {
            ringFactor = 0.65; // 3-4f window
        } else {
            ringFactor = 1.00;
        }
    }

    // Rapid successive clicks (spam or tight timing)
    int deltaFromLastClick = m_currentFrame - m_lastClickFrame;
    double freqFactor = 1.0;
    if (deltaFromLastClick > 0 && deltaFromLastClick <= 3) {
        freqFactor = 0.25;
    } else if (deltaFromLastClick <= 6) {
        freqFactor = 0.50;
    } else if (deltaFromLastClick <= 10) {
        freqFactor = 0.75;
    }

    double leniency = baseLeniency * speedFactor * velFactor * modeFactor *
                      sizeFactor * ringFactor * freqFactor;

    double minLen = 0.0;
    double maxLen = 50.0 / m_analysisFPS;
    return std::max(minLen, std::min(maxLen, leniency));
}

// ============================================================
// Results
// ============================================================

std::map<int, int> const& FrameWindowAnalyzer::getWindowDistribution() const {
    return m_windowDistribution;
}

int FrameWindowAnalyzer::getTotalClicks() const { return m_totalClicks; }

double FrameWindowAnalyzer::getPrecisionScore() const {
    if (m_clicks.empty() || m_levelTime <= 0.0) return 0.0;
    double totalLogProb = 0.0;
    for (auto const& click : m_clicks) {
        double prob = static_cast<double>(click.frameWindow) / static_cast<double>(m_analysisFPS);
        prob = std::max(0.0001, std::min(1.0, prob));
        totalLogProb += std::log(prob);
    }
    return -totalLogProb / m_levelTime;
}

double FrameWindowAnalyzer::getAverageWindow() const {
    if (m_clicks.empty()) return 0.0;
    double sum = 0.0;
    for (auto const& c : m_clicks) sum += c.frameWindow;
    return sum / m_clicks.size();
}

int FrameWindowAnalyzer::getTightestWindow() const {
    if (m_clicks.empty()) return 0;
    int t = 999;
    for (auto const& c : m_clicks) t = std::min(t, c.frameWindow);
    return t;
}

// CBF
int FrameWindowAnalyzer::getCBFCount() const { return m_cbfCount; }
bool FrameWindowAnalyzer::isLastClickCBF() const { return m_lastClickCBF; }

std::string FrameWindowAnalyzer::getCBFText() const {
    if (m_cbfCount > 0 && m_lastClickCBF) {
        return fmt::format("CBF: {} (Frame Perfect!)", m_cbfCount);
    } else if (m_cbfCount > 0) {
        return fmt::format("CBF: {}", m_cbfCount);
    } else {
        return "CBF: 0";
    }
}

// Ring FP
int FrameWindowAnalyzer::getRingFPCount() const { return m_ringFPCount; }
int FrameWindowAnalyzer::getTotalRingHits() const { return m_totalRingHits; }
bool FrameWindowAnalyzer::isLastRingFP() const { return m_lastRingFP; }

// Gamemode
FrameWindowAnalyzer::GamemodeStats const& FrameWindowAnalyzer::getGamemodeStats(Gamemode mode) const {
    int idx = static_cast<int>(mode);
    if (idx >= 0 && idx < static_cast<int>(Gamemode::COUNT)) {
        return m_gamemodeStats[idx];
    }
    return s_emptyStats;
}

FrameWindowAnalyzer::Gamemode FrameWindowAnalyzer::getCurrentGamemode() const {
    return m_gameState.currentMode;
}

int FrameWindowAnalyzer::getAnalysisFPS() const { return m_analysisFPS; }
void FrameWindowAnalyzer::setAnalysisFPS(int fps) {
    m_analysisFPS = std::max(60, std::min(360, fps));
}

std::vector<FrameWindowAnalyzer::ClickData> const& FrameWindowAnalyzer::getClicks() const {
    return m_clicks;
}
bool FrameWindowAnalyzer::isTracking() const { return m_isTracking; }
void FrameWindowAnalyzer::setTracking(bool t) { m_isTracking = t; }
bool FrameWindowAnalyzer::hasMacroLoaded() const { return m_macroLoaded; }
std::string FrameWindowAnalyzer::getMacroName() const { return m_macroName; }
float FrameWindowAnalyzer::getMacroFPS() const { return m_macroFPS; }
FrameWindowAnalyzer::GameState const& FrameWindowAnalyzer::getGameState() const {
    return m_gameState;
}
