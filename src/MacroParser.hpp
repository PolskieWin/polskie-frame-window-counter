#pragma once

#include <Geode/Geode.hpp>
#include <vector>
#include <string>

using namespace geode::prelude;

/**
 * MacroParser
 * 
 * Comprehensive universal parser for Geometry Dash macros & replays:
 * - GDReplay / Eclipse (.gdr, .gdr2) LEB128 binary, JSON, and MessagePack
 * - MegaHack (.mhr) binary/JSON and ReplayBot (.rply) binary
 * - Silico (.slc), TasBot (.tcm), and yBot / zBot (.ybot) JSON event streams
 * - MegaHack Replay JSON (.mhr.json, .mhr)
 * - xdBot (.txt / .csv / .json)
 * - xBot (.txt / .csv)
 * - TASBot (.json)
 * - ReplayBot (.replay, .rply)
 * - Generic JSON replay formats
 * - Plain text / CSV macros
 */
class MacroParser {
public:
    struct MacroInput {
        int frame = 0;          // Frame index
        double subFrame = 0.0;  // Fractional position within the frame
        double timeSeconds = 0.0;
        bool hasTimeSeconds = false;
        double windowFrames = 0.0;
        bool hasWindowFrames = false;
        bool isPress = true;    // true = press, false = release
        int player = 1;         // 1 = player1, 2 = player2
        int button = 1;         // 1 = jump, 2 = left, 3 = right
    };

    struct MacroData {
        std::string name;
        std::string author;
        float recordFPS = 240.0f;
        std::string botName;
        std::string gameVersion;
        std::vector<MacroInput> inputs;
        bool valid = false;
        std::string error;
    };

    enum class MacroFormat {
        ECLIPSE_GDR2,   // Eclipse Menu .gdr2
        GDREPLAY,        // GDReplay .gdr JSON or MessagePack event stream
        SILICO,          // Silico .slc event stream
        TASBOT_TCM,      // TasBot .tcm event stream
        YBOT,            // yBot / zBot .ybot action stream
        MHR_BINARY,     // MegaHack Replay Binary (.mhr)
        MHR_JSON,       // MegaHack Replay JSON (.mhr.json)
        XDBOT,          // xdBot .txt / .csv / .json
        XBOT,           // xBot .txt / .csv
        TASBOT,         // TASBot .json
        REPLAYBOT,      // ReplayBot .replay
        JSON,           // Generic JSON
        PLAIN_TEXT,     // Text / CSV format
        UNKNOWN
    };

    static MacroFormat detectFormat(std::string const& path);
    static MacroData parse(std::string const& path);

    static MacroData parseEclipseGDR2(std::string const& path);
    static MacroData parseGDReplay(std::string const& path);
    static MacroData parseSilico(std::string const& path);
    static MacroData parseTasBotTCM(std::string const& path);
    static MacroData parseYBot(std::string const& path);
    static MacroData parseEventJson(std::string const& path);
    static MacroData parseMHRBinary(std::string const& path);
    static MacroData parseMHRJson(std::string const& path);
    static MacroData parseXdBot(std::string const& path);
    static MacroData parseXbot(std::string const& path);
    static MacroData parseTasBot(std::string const& path);
    static MacroData parseReplayBot(std::string const& path);
    static MacroData parseJsonMacro(std::string const& path);
    static MacroData parsePlainText(std::string const& path);

    static std::vector<std::string> findMacroFiles(std::string const& directory);
    static std::vector<std::string> getDefaultMacroDirs();
};
