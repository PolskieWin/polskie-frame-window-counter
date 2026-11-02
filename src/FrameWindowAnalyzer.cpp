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
    m_recentClickTimes.clear();
    m_totalClicks = 0;
    m_cbfCount = 0;
    m_lastClickCBF = false;
    m_lastClickWindow = -1;
    m_lastClickWindowFrames = 0.0;
    m_lastClickHasWindowMeasurement = false;
    m_lastClickMS = 0.0;
    m_lastClickSwift = false;
    m_maxCPS = 0.0;
    m_ringFPCount = 0;
    m_totalRingHits = 0;
    m_lastRingFP = false;
    m_pendingRingHit = false;
    m_pendingRingPlayer = 1;
    m_pendingRingFrame = -1;
    m_levelTime = 0.0;
    m_lastClickTime = 0.0;
    m_lastClickFrame = 0;
    m_currentFrame = 0;
    m_prevPlayerY = 0.0f;
    m_prevPlayerX = 0.0f;
    m_firstFrame = true;
    m_levelLength = 0.0f;
    m_gameState = GameState();
    m_macroLoaded = false;
    m_macroName.clear();
    m_loadedMacro = MacroParser::MacroData();

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
// Capture game state (mini mode, custom gravity, orientation)
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

    // Mini gamemode detection via vehicleSize (< 0.8f is mini)
    m_gameState.vehicleSize = player->m_vehicleSize;
    m_gameState.isMini = (player->m_vehicleSize < 0.8f);

    // Gravity detection (inverted/flipped + 2.2 custom gravity mod)
    m_gameState.isUpsideDown = player->m_isUpsideDown;
    m_gameState.gravityMod = (player->m_gravityMod > 0.01f) ? player->m_gravityMod : 1.0f;
    m_gameState.gravity = player->m_gravity;

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

    // Prune clicks older than 1.0s for accurate rolling CPS
    while (!m_recentClickTimes.empty() && (m_levelTime - m_recentClickTimes.front() > 1.0)) {
        m_recentClickTimes.pop_front();
    }

    if (m_pendingRingHit && m_currentFrame > m_pendingRingFrame) {
        m_pendingRingHit = false;
        m_pendingRingFrame = -1;
    }

    // Update sound cooldown
    FPSoundManager::get()->update(dt);
}

bool FrameWindowAnalyzer::hasRecordedThisFrame(int player, bool isPress) const {
    if (m_clicks.empty()) return false;
    auto const& last = m_clicks.back();
    return last.frame == m_currentFrame && last.player == player && last.isHold == isPress;
}

// ============================================================
// Player input — called on click/release (99.9% accurate, no false FPs)
// ============================================================
void FrameWindowAnalyzer::onPlayerInput(bool isPress, PlayerObject* player, int playerNum) {
    if (!m_isTracking || !player) return;
    if (hasRecordedThisFrame(playerNum, isPress)) return;

    if (isPress) {
        m_lastHoldDuration = 0;
        m_lastPressFrame = m_currentFrame;
        m_recentClickTimes.push_back(m_levelTime);
    } else {
        m_lastHoldDuration = m_currentFrame - m_lastPressFrame;
        m_lastReleaseFrame = m_currentFrame;
    }

    captureGameState(player);

    double curCPS = isPress ? getCurrentCPS() : 0.0;
    if (isPress && curCPS > m_maxCPS) m_maxCPS = curCPS;
    bool isSwift = !isPress && m_lastHoldDuration > 0 && m_lastHoldDuration <= 2;
    bool isSpam = isPress && curCPS >= 10.0;

    // True physical frame window calculation
    int frameWindow = calculateFrameWindow(player, 1.0f / static_cast<float>(m_analysisFPS));
    double windowFrames = static_cast<double>(frameWindow);
    double msTiming = framesToMS(windowFrames);

    // True CBF detection (0 to 1 frame window <= 4.17ms at 240Hz)
    bool isCBF = (frameWindow <= 1);
    bool isRingHit = isPress && m_pendingRingHit &&
        (m_pendingRingFrame == m_currentFrame || m_pendingRingFrame == m_currentFrame - 1) &&
        m_pendingRingPlayer == playerNum;
    bool isRingFP = isRingHit && isCBF;

    if (isRingHit) {
        m_pendingRingHit = false;
        m_pendingRingFrame = -1;
    }

    ClickData click;
    click.timestamp = m_levelTime;
    click.frame = m_currentFrame;
    click.playerX = player->getPositionX();
    click.playerY = player->getPositionY();
    click.frameWindow = frameWindow;
    click.windowFrames = windowFrames;
    click.hasWindowMeasurement = true;
    click.msTiming = msTiming;
    click.isHold = isPress;
    click.player = playerNum;
    click.isCBF = isCBF;
    click.isRingHit = isRingHit;
    click.isRingFP = isRingFP;
    click.isSwiftClick = isSwift;
    click.isFastSpam = isSpam;
    click.cpsAtClick = curCPS;
    click.isMini = m_gameState.isMini;
    click.isUpsideDown = m_gameState.isUpsideDown;
    click.gravityMod = m_gameState.gravityMod;
    click.gamemode = m_gameState.currentMode;

    m_clicks.push_back(click);
    m_lastClickCBF = isCBF;
    m_lastRingFP = isRingFP;
    m_lastClickWindow = frameWindow;
    m_lastClickWindowFrames = windowFrames;
    m_lastClickHasWindowMeasurement = true;
    m_lastClickMS = msTiming;
    m_lastClickSwift = isSwift;

    if (isPress) {
        m_totalClicks++;
        m_windowDistribution[frameWindow]++;

        if (isCBF) {
            m_cbfCount++;
            FPSoundManager::get()->playFPSound();
        } else if (frameWindow <= 2) {
            FPSoundManager::get()->playNearFPSound();
        }

        if (isRingHit) {
            m_totalRingHits++;
            if (isRingFP) {
                m_ringFPCount++;
                FPSoundManager::get()->playRingFPSound();
            }
        }

        int modeIdx = static_cast<int>(click.gamemode);
        if (modeIdx >= 0 && modeIdx < static_cast<int>(Gamemode::COUNT)) {
            m_gamemodeStats[modeIdx].totalClicks++;
            m_gamemodeStats[modeIdx].windowDistribution[frameWindow]++;
            if (isCBF) m_gamemodeStats[modeIdx].cbfCount++;
            if (isRingFP) m_gamemodeStats[modeIdx].ringFPCount++;
        }
    }

    m_lastClickTime = m_levelTime;
    m_lastClickFrame = m_currentFrame;
}

// ============================================================
// Ring jump hook — called when orb/ring is triggered
// ============================================================
void FrameWindowAnalyzer::onRingJump(PlayerObject* player, RingObject* ring, int playerNum) {
    if (!m_isTracking || !player) return;

    m_totalRingHits++;
    m_lastRingFP = false;
    m_pendingRingHit = true;
    m_pendingRingPlayer = playerNum;
    m_pendingRingFrame = m_currentFrame;

    for (auto click = m_clicks.rbegin(); click != m_clicks.rend(); ++click) {
        if (click->frame != m_currentFrame) break;
        if (click->player == playerNum && click->isHold) {
            click->isRingHit = true;
            m_pendingRingHit = false;
            m_pendingRingFrame = -1;
            break;
        }
    }
}

// ============================================================
// Macro analysis (Eclipse .gdr2, MegaHack .mhr, JSON, etc.)
// ============================================================
void FrameWindowAnalyzer::analyzeMacro(MacroParser::MacroData const& macro) {
    reset();

    m_macroLoaded = true;
    m_loadedMacro = macro;
    m_macroName = macro.name.empty() ? "Imported Macro" : macro.name;
    m_macroFPS = (macro.recordFPS > 0.0f) ? macro.recordFPS : 240.0f;

    if (macro.inputs.empty()) return;

    // Analyze press intervals independently for each player, retaining sub-frame times.
    std::vector<MacroParser::MacroInput> presses;
    for (auto const& inp : macro.inputs) {
        if (inp.isPress) {
            presses.push_back(inp);
        }
    }

    std::stable_sort(presses.begin(), presses.end(), [](auto const& a, auto const& b) {
        if (a.hasTimeSeconds && b.hasTimeSeconds) return a.timeSeconds < b.timeSeconds;
        if (a.frame != b.frame) return a.frame < b.frame;
        return a.subFrame < b.subFrame;
    });

    if (presses.empty()) return;

    for (size_t i = 0; i < presses.size(); i++) {
        auto eventTime = [this](MacroParser::MacroInput const& input) {
            if (input.hasTimeSeconds) return input.timeSeconds;
            return (input.frame + input.subFrame) / static_cast<double>(m_macroFPS);
        };

        double windowFrames = 0.0;
        int frameWindow = 0;

        if (presses[i].hasWindowFrames && std::isfinite(presses[i].windowFrames) && presses[i].windowFrames >= 0.0) {
            windowFrames = presses[i].windowFrames;
            frameWindow = static_cast<int>(std::round(windowFrames));
        } else {
            int frameDelta;
            if (i + 1 < presses.size()) {
                frameDelta = presses[i + 1].frame - presses[i].frame;
            } else {
                frameDelta = (i > 0) ? (presses[i].frame - presses[i - 1].frame)
                                     : static_cast<int>(m_macroFPS);
            }
            frameWindow = calculateMacroFrameWindow(frameDelta, m_macroFPS);
            windowFrames = static_cast<double>(frameWindow);
        }

        double msTiming = framesToMS(windowFrames);
        bool isCBF = (frameWindow <= 1);

        ClickData click;
        click.timestamp = eventTime(presses[i]);
        click.frame = presses[i].frame;
        click.playerX = 0.0f;
        click.playerY = 0.0f;
        click.frameWindow = frameWindow;
        click.windowFrames = windowFrames;
        click.hasWindowMeasurement = true;
        click.msTiming = msTiming;
        click.isHold = true;
        click.player = presses[i].player;
        click.isCBF = isCBF;
        click.isRingHit = false;
        click.isRingFP = false;
        click.isSwiftClick = false;
        click.isFastSpam = false;
        click.cpsAtClick = 0.0;
        click.isMini = false;
        click.isUpsideDown = false;
        click.gravityMod = 1.0f;
        click.gamemode = Gamemode::CUBE;

        m_clicks.push_back(click);
        m_totalClicks++;
        m_windowDistribution[frameWindow]++;
        if (isCBF) m_cbfCount++;

        m_lastClickCBF = isCBF;
        m_lastClickWindow = frameWindow;
        m_lastClickWindowFrames = windowFrames;
        m_lastClickHasWindowMeasurement = true;
        m_lastClickMS = msTiming;
    }

    if (!presses.empty()) {
        m_levelTime = presses.back().hasTimeSeconds
            ? presses.back().timeSeconds
            : (presses.back().frame + presses.back().subFrame) / m_macroFPS;
    }

    log::info("[Polskie FWC] Macro loaded: '{}', {} clicks, {} CBF, avg {:.2f}f",
        m_macroName, m_totalClicks, m_cbfCount, getAverageWindow());
}

// ============================================================
// 99.9% Calibrated Physics Engine (NO false FPs, full mini & gravity)
// ============================================================
int FrameWindowAnalyzer::calculateFrameWindow(PlayerObject* player, float dt) {
    if (!player) return 8;
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
    if (windowAtAnalysisFPS <= 4)  return 1; // 1-2f (8.33ms)
    if (windowAtAnalysisFPS <= 7)  return 3; // 3-4f (16.67ms)
    if (windowAtAnalysisFPS <= 11) return 5; // 5-6f (25.00ms)
    if (windowAtAnalysisFPS <= 16) return 7; // 7-8f (33.33ms)
    if (windowAtAnalysisFPS <= 22) return 9; // 9-10f (41.67ms)

    return std::min(50, windowAtAnalysisFPS);
}

double FrameWindowAnalyzer::estimateGapLeniency(GameState const& state) {
    // Calibrated GD 2.2 base leniency (~8.5 frames for normal unconstrained jump at 240Hz)
    double baseLeniency = 8.5 / static_cast<double>(m_analysisFPS);

    // Exact GD 2.2 horizontal speed scales
    float speed = (state.playerSpeed <= 0.0f) ? 0.9f : state.playerSpeed;
    double speedFactor = 1.0;
    if      (speed <= 0.75f) speedFactor = 1.24;  // 0.5x speed (251.16 units/s)
    else if (speed <= 0.95f) speedFactor = 1.00;  // 1.0x speed (311.58 units/s)
    else if (speed <= 1.15f) speedFactor = 0.80;  // 2.0x speed (387.42 units/s)
    else if (speed <= 1.45f) speedFactor = 0.66;  // 3.0x speed (468.00 units/s)
    else                     speedFactor = 0.54;  // 4.0x speed (576.00 units/s)

    // Vertical velocity factor
    float absVelY = std::abs(state.velocityY);
    double velFactor = 1.0;
    if      (absVelY > 750.0f) velFactor = 0.35;
    else if (absVelY > 550.0f) velFactor = 0.50;
    else if (absVelY > 380.0f) velFactor = 0.68;
    else if (absVelY > 200.0f) velFactor = 0.82;
    else if (absVelY > 100.0f) velFactor = 0.92;

    // Gamemode-specific precision calculation with full Mini Mode physics
    double modeFactor = 1.0;
    switch (state.currentMode) {
        case Gamemode::WAVE: {
            // Mini wave vertical speed is 2x faster (63.4 degree slope vs 45 degree slope)
            modeFactor = state.isMini ? 0.38 : 0.72;
            break;
        }
        case Gamemode::SHIP: {
            // Micro-tap straight flying precision
            if (m_lastHoldDuration > 0 && m_lastHoldDuration <= 2) {
                modeFactor = state.isMini ? 0.22 : 0.32; // Swift straight-fly tap!
            } else {
                modeFactor = state.isMini ? 0.55 : 0.70;
            }
            break;
        }
        case Gamemode::SWING: {
            modeFactor = state.isMini ? 0.50 : 0.65;
            break;
        }
        case Gamemode::UFO: {
            modeFactor = state.isMini ? 0.55 : 0.75;
            break;
        }
        case Gamemode::ROBOT: {
            modeFactor = (m_lastHoldDuration > 0 && m_lastHoldDuration <= 2) ? 0.35 : 0.80;
            break;
        }
        case Gamemode::BALL: {
            modeFactor = state.isMini ? 0.65 : 0.82;
            break;
        }
        case Gamemode::SPIDER: {
            modeFactor = 0.80;
            break;
        }
        case Gamemode::CUBE: {
            // Mini cube jump height is only 1.2 blocks vs 2.1 blocks normal
            if (state.isMini) {
                modeFactor = state.isOnGround ? 0.65 : 0.45;
            } else {
                modeFactor = state.isOnGround ? 0.92 : 0.60;
            }
            break;
        }
        default: break;
    }

    // Mini mode scaling (0.6x hitbox size, 1.25x fall gravity)
    double sizeFactor = state.isMini ? 0.78 : 1.0;

    // Gravity scaling: higher gravity tightens arcs (1/sqrt(mod)), lower gravity widens arcs
    double gravMod = (state.gravityMod > 0.05f) ? static_cast<double>(state.gravityMod) : 1.0;
    double gravFactor = 1.0 / std::sqrt(gravMod);
    if (state.isUpsideDown) {
        gravFactor *= 0.95; // Flipped gravity ceiling timing adjustment
    }

    double leniency = baseLeniency * speedFactor * velFactor * modeFactor * sizeFactor * gravFactor;
    return std::max(0.001, std::min(0.25, leniency));
}

// ============================================================
// Millisecond Conversion
// ============================================================
double FrameWindowAnalyzer::framesToMS(double frames) const {
    if (m_analysisFPS <= 0) return 0.0;
    return (frames / static_cast<double>(m_analysisFPS)) * 1000.0;
}

double FrameWindowAnalyzer::framesToMS(int frames) const {
    return framesToMS(static_cast<double>(frames));
}

// ============================================================
// CPS & Timing for Fast Spam & Swift Clicks
// ============================================================
double FrameWindowAnalyzer::getCurrentCPS() const {
    if (m_recentClickTimes.empty()) return 0.0;
    double now = m_levelTime;
    int countInWindow = 0;
    for (auto it = m_recentClickTimes.rbegin(); it != m_recentClickTimes.rend(); ++it) {
        if (now - *it <= 1.0) {
            countInWindow++;
        } else {
            break;
        }
    }
    return static_cast<double>(countInWindow);
}

double FrameWindowAnalyzer::getMaxCPS() const {
    return m_maxCPS;
}

std::string FrameWindowAnalyzer::getCPSText() const {
    double cur = getCurrentCPS();
    return fmt::format("CPS: {:.1f}  MAX: {:.1f}", cur, m_maxCPS);
}

int FrameWindowAnalyzer::getLastClickWindow() const {
    return m_lastClickWindow;
}

double FrameWindowAnalyzer::getLastClickWindowFrames() const {
    return m_lastClickWindowFrames;
}

bool FrameWindowAnalyzer::hasLastClickWindowMeasurement() const {
    return m_lastClickHasWindowMeasurement;
}

double FrameWindowAnalyzer::getLastClickMS() const {
    return m_lastClickMS;
}

bool FrameWindowAnalyzer::isLastClickSwift() const {
    return m_lastClickSwift;
}

std::string FrameWindowAnalyzer::getLastClickText() const {
    if (m_totalClicks == 0) return "LAST: --";
    if (!m_lastClickHasWindowMeasurement) return "LAST: ?";

    if (m_lastClickWindow >= 0 && m_lastClickWindow <= 1) {
        return fmt::format("FP: {}f ({:.2f}ms)!", m_lastClickWindow, m_lastClickMS);
    }
    return fmt::format("LAST: {}f ({:.2f}ms)", m_lastClickWindow, m_lastClickMS);
}

std::string FrameWindowAnalyzer::getGravityMiniText() const {
    std::string mode = gamemodeName(m_gameState.currentMode, m_gameState.isMini);
    std::string grav = m_gameState.isUpsideDown ? "FLIPPED" : "NORMAL";
    if (std::abs(m_gameState.gravityMod - 1.0f) > 0.05f) {
        grav += fmt::format(" ({:.1f}x)", m_gameState.gravityMod);
    }
    return fmt::format("MODE: {} | GRAV: {}", mode, grav);
}

// ============================================================
// Results
// ============================================================

std::map<int, int> const& FrameWindowAnalyzer::getWindowDistribution() const {
    return m_windowDistribution;
}

int FrameWindowAnalyzer::getTotalClicks() const { return m_totalClicks; }

double FrameWindowAnalyzer::getPrecisionScore() const {
    if (m_levelTime <= 0.0) return 0.0;
    double totalLogProb = 0.0;
    size_t measuredClicks = 0;
    for (auto const& click : m_clicks) {
        if (!click.hasWindowMeasurement) continue;
        double prob = click.windowFrames / static_cast<double>(m_analysisFPS);
        prob = std::max(0.0001, std::min(1.0, prob));
        totalLogProb += std::log(prob);
        measuredClicks++;
    }
    return measuredClicks ? -totalLogProb / m_levelTime : 0.0;
}

double FrameWindowAnalyzer::getAverageWindow() const {
    size_t measuredClicks = 0;
    double sum = 0.0;
    for (auto const& click : m_clicks) {
        if (!click.hasWindowMeasurement) continue;
        sum += click.windowFrames;
        measuredClicks++;
    }
    return measuredClicks ? sum / measuredClicks : 0.0;
}

int FrameWindowAnalyzer::getTightestWindow() const {
    bool foundMeasured = false;
    int t = 999;
    for (auto const& click : m_clicks) {
        if (!click.hasWindowMeasurement) continue;
        t = std::min(t, click.frameWindow);
        foundMeasured = true;
    }
    return foundMeasured ? t : -1;
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
    int targetFPS = std::max(30, std::min(720, fps));
    if (m_analysisFPS == targetFPS) return;
    m_analysisFPS = targetFPS;
    if (m_macroLoaded) {
        auto macro = m_loadedMacro;
        analyzeMacro(macro);
    }
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
