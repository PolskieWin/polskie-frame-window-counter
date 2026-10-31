#pragma once

#include <Geode/Geode.hpp>
#include <vector>
#include <map>
#include <string>
#include "MacroParser.hpp"

using namespace geode::prelude;

/**
 * FrameWindowAnalyzer v3 — Polskie Edition
 * 
 * Core analysis engine with:
 * - CBF (Click Before Frame) tracking = frame perfect detection
 * - Ring/orb frame-perfect detection
 * - Per-gamemode click statistics
 * - All FPS analysis for all gamemodes
 * - Bot macro import support
 */
class FrameWindowAnalyzer {
public:
    // All GD gamemodes
    enum class Gamemode {
        CUBE, SHIP, BALL, UFO, WAVE, ROBOT, SPIDER, SWING,
        COUNT
    };

    static std::string gamemodeName(Gamemode mode) {
        switch (mode) {
            case Gamemode::CUBE:   return "Cube";
            case Gamemode::SHIP:   return "Ship";
            case Gamemode::BALL:   return "Ball";
            case Gamemode::UFO:    return "UFO";
            case Gamemode::WAVE:   return "Wave";
            case Gamemode::ROBOT:  return "Robot";
            case Gamemode::SPIDER: return "Spider";
            case Gamemode::SWING:  return "Swing";
            default:               return "Unknown";
        }
    }

    struct ClickData {
        double timestamp;
        int frame;
        float playerX;
        float playerY;
        int frameWindow;
        bool isHold;            // press or release
        int player;             // 1 or 2
        bool isCBF;             // Click Before Frame = frame perfect!
        bool isRingHit;         // Was this a ring/orb interaction
        bool isRingFP;          // Ring hit that was frame-perfect
        Gamemode gamemode;      // Which gamemode this click was in
    };

    // Per-gamemode statistics
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

    // Live gameplay
    void onUpdate(float dt, PlayerObject* player, float levelLength);
    void onPlayerInput(bool isPress, PlayerObject* player, int playerNum = 1);

    // Ring/orb interaction tracking
    void onRingJump(PlayerObject* player, RingObject* ring);

    // Macro analysis
    void analyzeMacro(MacroParser::MacroData const& macro);

    // === Results ===
    std::map<int, int> const& getWindowDistribution() const;
    int getTotalClicks() const;
    double getPrecisionScore() const;
    double getAverageWindow() const;
    int getTightestWindow() const;

    // CBF (Click Before Frame = Frame Perfect)
    int getCBFCount() const;       // Total frame-perfect clicks
    bool isLastClickCBF() const;   // Was the most recent click frame-perfect?
    std::string getCBFText() const; // "CBF: X (Frame Perfect!)" or "CBF: X"

    // Ring frame-perfect
    int getRingFPCount() const;
    int getTotalRingHits() const;
    bool isLastRingFP() const;

    // Per-gamemode stats
    GamemodeStats const& getGamemodeStats(Gamemode mode) const;
    Gamemode getCurrentGamemode() const;

    // Settings
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
    int calculateMacroFrameWindow(int frameDelta, float macroFPS);
    double estimateGapLeniency(GameState const& state);
    void captureGameState(PlayerObject* player);
    Gamemode detectGamemode(PlayerObject* player);

    static FrameWindowAnalyzer* s_instance;

    std::vector<ClickData> m_clicks;
    std::map<int, int> m_windowDistribution;
    GameState m_gameState;

    // Per-gamemode tracking
    GamemodeStats m_gamemodeStats[static_cast<int>(Gamemode::COUNT)];
    static GamemodeStats s_emptyStats;

    int m_analysisFPS = 240;
    int m_totalClicks = 0;
    bool m_isTracking = false;

    // CBF tracking
    int m_cbfCount = 0;            // Total frame-perfect clicks
    bool m_lastClickCBF = false;

    // Ring FP tracking
    int m_ringFPCount = 0;
    int m_totalRingHits = 0;
    bool m_lastRingFP = false;
    bool m_pendingRingHit = false;  // Flag set when ringJump is called

    // Timing
    double m_levelTime = 0.0;
    double m_lastClickTime = 0.0;
    int m_lastClickFrame = 0;
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
};
