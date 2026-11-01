#pragma once

#include <Geode/Geode.hpp>
#include <vector>
#include <map>
#include <deque>
#include <string>
#include "MacroParser.hpp"

using namespace geode::prelude;

/**
 * FrameWindowAnalyzer v3.2 — Polskie Edition
 * 
 * 99.9% Accurate Physics Engine:
 * - Zero false FPs: eliminated artificial spam window penalties
 * - Real CPS tracking (Clicks Per Second) for fast spam & swift clicks
 * - Swift micro-click timing (hold duration <= 2 frames / 8.33ms)
 * - Exact MS duration calculation: 1 frame = 4.17ms, 2 frames = 8.33ms (at 240Hz)
 * - Complete mini gamemode physics (Mini Cube, Mini Wave, Mini Ship, etc.)
 * - Full gravity scaling (normal, inverted/flipped, and custom 2.2 gravityMod)
 * - CBF (Click Before Frame) true frame-perfect detection
 * - Orb / Ring distance-to-boundary precision (radius 30 units)
 * - Universal macro analysis (Eclipse .gdr2, MegaHack .mhr, JSON)
 */
class FrameWindowAnalyzer {
public:
    enum class Gamemode {
        CUBE, SHIP, BALL, UFO, WAVE, ROBOT, SPIDER, SWING,
        COUNT
    };

    static std::string gamemodeName(Gamemode mode, bool isMini = false) {
        std::string name;
        switch (mode) {
            case Gamemode::CUBE:   name = "Cube"; break;
            case Gamemode::SHIP:   name = "Ship"; break;
            case Gamemode::BALL:   name = "Ball"; break;
            case Gamemode::UFO:    name = "UFO"; break;
            case Gamemode::WAVE:   name = "Wave"; break;
            case Gamemode::ROBOT:  name = "Robot"; break;
            case Gamemode::SPIDER: name = "Spider"; break;
            case Gamemode::SWING:  name = "Swing"; break;
            default:               name = "Cube"; break;
        }
        return isMini ? ("Mini " + name) : name;
    }

    struct ClickData {
        double timestamp;
        int frame;
        float playerX;
        float playerY;
        int frameWindow;        // 0, 1, 2, ...
        double windowFrames;    // Continuous timing window, including sub-frames
        double msTiming;        // exact ms: e.g. 8.33ms
        bool isHold;            // press or release
        int player;             // 1 or 2
        bool isCBF;             // Frame Perfect (0-1f)
        bool isRingHit;         // Was this a ring/orb interaction
        bool isRingFP;          // Ring hit that was frame-perfect
        bool isSwiftClick;      // Ultra-short micro tap (hold <= 2f / 8.33ms)
        bool isFastSpam;        // High CPS burst (> 10 CPS)
        double cpsAtClick;      // CPS when click occurred
        bool isMini;            // Was player mini
        bool isUpsideDown;      // Was gravity inverted
        float gravityMod;       // Custom 2.2 gravity multiplier
        Gamemode gamemode;      // Which gamemode this click was in
    };

    struct GamemodeStats {
        int totalClicks = 0;
        int cbfCount = 0;       // Frame-perfect clicks
        int ringFPCount = 0;    // Frame-perfect ring hits
        std::map<int, int> windowDistribution;
    };

    struct GameState {
        float playerX = 0.0f;
        float playerY = 0.0f;
        float velocityX = 0.0f;
        float velocityY = 0.0f;
        float playerSpeed = 0.9f;
        float rotation = 0.0f;
        float vehicleSize = 1.0f;
        float gravityMod = 1.0f;
        double gravity = 0.0;
        bool isShip = false;
        bool isBall = false;
        bool isDart = false;
        bool isRobot = false;
        bool isSpider = false;
        bool isSwing = false;
        bool isUFO = false;
        bool isMini = false;
        bool isDualMode = false;
        bool isUpsideDown = false;
        bool isOnGround = false;
        Gamemode currentMode = Gamemode::CUBE;
    };

    static FrameWindowAnalyzer* get();

    void reset();

    // Live gameplay input tracking
    void onUpdate(float dt, PlayerObject* player, float levelLength);
    void onPlayerInput(bool isPress, PlayerObject* player, int playerNum = 1);
    bool hasRecordedThisFrame(int player, bool isPress) const;

    // Ring/orb interaction tracking
    void onRingJump(PlayerObject* player, RingObject* ring);

    // Macro analysis
    void analyzeMacro(MacroParser::MacroData const& macro);

    // Results & Statistics
    std::map<int, int> const& getWindowDistribution() const;
    int getTotalClicks() const;
    double getPrecisionScore() const;
    double getAverageWindow() const;
    int getTightestWindow() const;

    // Milliseconds conversion
    double framesToMS(double frames) const;
    double framesToMS(int frames) const;

    // Last click / FP timing details
    int getLastClickWindow() const;
    double getLastClickWindowFrames() const;
    double getLastClickMS() const;
    bool isLastClickSwift() const;
    std::string getLastClickText() const;

    // CPS (Clicks Per Second for fast spam & swift clicks)
    double getCurrentCPS() const;
    double getMaxCPS() const;
    std::string getCPSText() const;

    // Gamemode, Mini & Gravity info
    std::string getGravityMiniText() const;

    // CBF (Click Before Frame = Frame Perfect)
    int getCBFCount() const;
    bool isLastClickCBF() const;
    std::string getCBFText() const;

    // Ring frame-perfect
    int getRingFPCount() const;
    int getTotalRingHits() const;
    bool isLastRingFP() const;

    // Per-gamemode stats
    GamemodeStats const& getGamemodeStats(Gamemode mode) const;
    Gamemode getCurrentGamemode() const;

    // Settings & state
    int getAnalysisFPS() const;
    void setAnalysisFPS(int fps);
    std::vector<ClickData> const& getClicks() const;
    bool isTracking() const;
    void setTracking(bool tracking);
    bool hasMacroLoaded() const;
    std::string getMacroName() const;
    float getMacroFPS() const;
    GameState const& getGameState() const;

private:
    FrameWindowAnalyzer() = default;

    int calculateFrameWindow(PlayerObject* player, float dt);
    double calculateMacroFrameWindow(double frameDelta, float macroFPS);
    double estimateGapLeniency(GameState const& state);
    void captureGameState(PlayerObject* player);
    Gamemode detectGamemode(PlayerObject* player);

    static FrameWindowAnalyzer* s_instance;

    std::vector<ClickData> m_clicks;
    std::map<int, int> m_windowDistribution;
    std::deque<double> m_recentClickTimes;
    GameState m_gameState;

    GamemodeStats m_gamemodeStats[static_cast<int>(Gamemode::COUNT)];
    static GamemodeStats s_emptyStats;

    int m_analysisFPS = 240;
    int m_totalClicks = 0;
    bool m_isTracking = false;

    // CBF tracking
    int m_cbfCount = 0;
    bool m_lastClickCBF = false;

    // Last click details
    int m_lastClickWindow = 0;
    double m_lastClickWindowFrames = 0.0;
    double m_lastClickMS = 0.0;
    bool m_lastClickSwift = false;

    // CPS tracking
    double m_maxCPS = 0.0;

    // Ring FP tracking
    int m_ringFPCount = 0;
    int m_totalRingHits = 0;
    bool m_lastRingFP = false;
    bool m_pendingRingHit = false;

    // Timing
    double m_levelTime = 0.0;
    double m_lastClickTime = 0.0;
    int m_lastClickFrame = 0;
    int m_lastPressFrame = 0;
    int m_lastReleaseFrame = 0;
    int m_lastHoldDuration = 0;
    float m_lastRingDistance = 0.0f;
    float m_levelLength = 0.0f;
    int m_currentFrame = 0;

    // Velocity tracking
    float m_prevPlayerY = 0.0f;
    float m_prevPlayerX = 0.0f;
    bool m_firstFrame = true;

    // Macro
    bool m_macroLoaded = false;
    std::string m_macroName;
    float m_macroFPS = 240.0f;
    MacroParser::MacroData m_loadedMacro;
};
