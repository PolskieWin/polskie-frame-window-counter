#pragma once

#include <Geode/Geode.hpp>
#include <vector>
#include <string>

using namespace geode::prelude;

/**
 * MacroParser
 * 
 * Supports:
 * - Eclipse Menu / GDR2 (.gdr2, .gdr) — binary replay format ("esplice")
 * - MegaHack Replay (.mhr, .mhr.json)
 * - Generic JSON / TASBot / xdBot (.json)
 */
class MacroParser {
public:
    struct MacroInput {
        int frame;          // Frame number
        bool isPress;       // true = press, false = release
        int player;         // 1 = player1, 2 = player2
        int button;         // 1 = jump, 2 = left, 3 = right
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
        ECLIPSE_GDR2,   // Eclipse Menu .gdr2 / .gdr
        MHR_JSON,       // MegaHack Replay
        JSON,           // TASBot / generic JSON
        UNKNOWN
    };

    static MacroFormat detectFormat(std::string const& path);
    static MacroData parse(std::string const& path);

    static MacroData parseEclipseGDR2(std::string const& path);
    static MacroData parseMHRJson(std::string const& path);
    static MacroData parseGenericJson(std::string const& path);

    static std::vector<std::string> findMacroFiles(std::string const& directory);
    static std::vector<std::string> getDefaultMacroDirs();
};
