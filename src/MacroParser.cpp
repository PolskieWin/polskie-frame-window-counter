#include "MacroParser.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <Geode/utils/file.hpp>

namespace fs = std::filesystem;

// Helper: read unsigned varint from stream
static uint64_t readVarint(std::istream& stream) {
    uint64_t result = 0;
    int shift = 0;
    uint8_t byte = 0;
    while (stream.read(reinterpret_cast<char*>(&byte), 1)) {
        result |= static_cast<uint64_t>(byte & 0x7F) << shift;
        if (!(byte & 0x80)) break;
        shift += 7;
        if (shift >= 64) break;
    }
    return result;
}

MacroParser::MacroFormat MacroParser::detectFormat(std::string const& path) {
    std::string ext = fs::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    std::string filename = fs::path(path).filename().string();
    std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);

    // Read initial bytes to inspect content
    std::ifstream file(path, std::ios::binary);
    if (file.is_open()) {
        char buf[16] = {};
        file.read(buf, 16);
        std::streamsize bytesRead = file.gcount();

        // Check for GDR2 magic header ("GDR")
        if (bytesRead >= 3 && buf[0] == 'G' && buf[1] == 'D' && buf[2] == 'R') {
            return MacroFormat::ECLIPSE_GDR2;
        }

        // Check for JSON magic '{' or '['
        for (int i = 0; i < bytesRead; i++) {
            char c = buf[i];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') continue;
            if (c == '{' || c == '[') {
                return (filename.ends_with(".mhr.json") || ext == ".mhr")
                    ? MacroFormat::MHR_JSON
                    : MacroFormat::JSON;
            }
            break;
        }
    }

    if (ext == ".gdr2" || ext == ".gdr") {
        return MacroFormat::ECLIPSE_GDR2;
    }
    if (filename.ends_with(".mhr.json")) {
        return MacroFormat::MHR_JSON;
    }
    if (ext == ".mhr") {
        return MacroFormat::MHR_BINARY;
    }
    if (ext == ".json") {
        return MacroFormat::JSON;
    }
    if (ext == ".txt" || ext == ".csv") {
        return MacroFormat::PLAIN_TEXT;
    }

    return MacroFormat::UNKNOWN;
}

MacroParser::MacroData MacroParser::parse(std::string const& path) {
    if (!fs::exists(path)) {
        MacroData err;
        err.valid = false;
        err.error = "File does not exist: " + path;
        return err;
    }

    auto format = detectFormat(path);
    MacroData data;

    switch (format) {
        case MacroFormat::ECLIPSE_GDR2:
            data = parseEclipseGDR2(path);
            break;
        case MacroFormat::MHR_JSON:
        case MacroFormat::JSON:
            data = parseJsonMacro(path);
            break;
        case MacroFormat::MHR_BINARY:
            data = parseMHRBinary(path);
            break;
        case MacroFormat::PLAIN_TEXT:
            data = parsePlainText(path);
            break;
        default:
            // Attempt cascading parsers
            data = parseJsonMacro(path);
            break;
    }

    // Cascading fallbacks if primary parser produced no inputs
    if (!data.valid) {
        if (format != MacroFormat::JSON && format != MacroFormat::MHR_JSON) {
            auto jsonAttempt = parseJsonMacro(path);
            if (jsonAttempt.valid) return jsonAttempt;
        }
        if (format != MacroFormat::ECLIPSE_GDR2) {
            auto gdrAttempt = parseEclipseGDR2(path);
            if (gdrAttempt.valid) return gdrAttempt;
        }
        if (format != MacroFormat::MHR_BINARY) {
            auto mhrAttempt = parseMHRBinary(path);
            if (mhrAttempt.valid) return mhrAttempt;
        }
        if (format != MacroFormat::PLAIN_TEXT) {
            auto txtAttempt = parsePlainText(path);
            if (txtAttempt.valid) return txtAttempt;
        }
    }

    return data;
}

// ============================================================
// Eclipse Menu (.gdr2 / .gdr) — GDReplayFormat binary parser
// ============================================================
MacroParser::MacroData MacroParser::parseEclipseGDR2(std::string const& path) {
    MacroData data;
    data.botName = "Eclipse Menu (GDR2)";
    data.name = fs::path(path).stem().string();

    try {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            data.error = "Could not open Eclipse macro file: " + path;
            return data;
        }

        char magic[4] = {};
        file.read(magic, 4);

        float fps = 240.0f;
        file.read(reinterpret_cast<char*>(&fps), sizeof(float));
        if (fps >= 30.0f && fps <= 1000.0f) {
            data.recordFPS = fps;
        } else {
            data.recordFPS = 240.0f;
        }

        uint64_t inputCount = readVarint(file);
        if (inputCount == 0 || inputCount > 1000000) {
            // Check offset 3 (standard 3-byte 'GDR' header)
            file.clear();
            file.seekg(3, std::ios::beg);
            uint8_t version = 0;
            file.read(reinterpret_cast<char*>(&version), 1);
            file.read(reinterpret_cast<char*>(&fps), sizeof(float));
            if (fps >= 30.0f && fps <= 1000.0f) {
                data.recordFPS = fps;
            }
            inputCount = readVarint(file);
        }

        int currentFrame = 0;
        for (uint64_t i = 0; i < inputCount && !file.eof(); i++) {
            uint64_t delta = readVarint(file);
            currentFrame += static_cast<int>(delta);

            uint8_t flags = 0;
            file.read(reinterpret_cast<char*>(&flags), 1);

            MacroInput action;
            action.frame = currentFrame;
            action.isPress = (flags & 0x01) != 0;
            action.player = (flags & 0x02) ? 2 : 1;
            action.button = (flags >> 2) & 0x03;
            if (action.button == 0) action.button = 1;

            data.inputs.push_back(action);
        }

        if (data.inputs.empty()) {
            data.error = "No frame inputs found in Eclipse GDR2 file";
            data.valid = false;
        } else {
            data.valid = true;
            log::info("[Polskie FWC] Loaded Eclipse GDR2 macro: {} inputs at {:.0f} FPS",
                data.inputs.size(), data.recordFPS);
        }

    } catch (std::exception const& e) {
        data.error = std::string("Error parsing Eclipse GDR2: ") + e.what();
    }

    return data;
}

// ============================================================
// MegaHack Replay (.mhr) binary parser
// ============================================================
MacroParser::MacroData MacroParser::parseMHRBinary(std::string const& path) {
    MacroData data;
    data.botName = "MegaHack Replay (Binary)";
    data.name = fs::path(path).stem().string();

    try {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            data.error = "Could not open MegaHack binary file: " + path;
            return data;
        }

        // MegaHack binary header: FPS at offset 0 or 4
        float fps = 240.0f;
        file.read(reinterpret_cast<char*>(&fps), sizeof(float));
        if (fps >= 30.0f && fps <= 1000.0f) {
            data.recordFPS = fps;
        } else {
            data.recordFPS = 240.0f;
            file.seekg(0, std::ios::beg);
        }

        // Packed input stream: 32-bit frame + byte down + byte player + byte button
        struct RawAction {
            uint32_t frame;
            uint8_t down;
            uint8_t player;
            uint8_t button;
        };

        RawAction raw;
        while (file.read(reinterpret_cast<char*>(&raw), sizeof(RawAction))) {
            if (raw.frame > 50000000) break; // End of valid data stream
            MacroInput inp;
            inp.frame = static_cast<int>(raw.frame);
            inp.isPress = (raw.down != 0);
            inp.player = (raw.player == 2) ? 2 : 1;
            inp.button = (raw.button > 0) ? raw.button : 1;
            data.inputs.push_back(inp);
        }

        if (!data.inputs.empty()) {
            data.valid = true;
            log::info("[Polskie FWC] Loaded MegaHack binary macro: {} inputs at {:.0f} FPS",
                data.inputs.size(), data.recordFPS);
        } else {
            data.error = "No binary input actions found in MegaHack .mhr file";
        }

    } catch (std::exception const& e) {
        data.error = std::string("Error parsing MegaHack binary: ") + e.what();
    }

    return data;
}

// ============================================================
// Unified JSON Macro Parser (MegaHack JSON, TASBot, xdBot, etc.)
// ============================================================
MacroParser::MacroData MacroParser::parseJsonMacro(std::string const& path) {
    MacroData data;
    data.botName = "JSON Bot Macro";
    data.name = fs::path(path).stem().string();

    try {
        auto result = file::readString(fs::path(path));
        if (!result) {
            data.error = "Failed to read file: " + path;
            return data;
        }

        std::string content = result.unwrap();
        auto jsonRes = matjson::parse(content);
        if (!jsonRes) {
            data.error = "Invalid JSON syntax: " + jsonRes.unwrapErr().message;
            return data;
        }
        auto json = jsonRes.unwrap();

        // Extract metadata
        if (json.contains("name")) data.name = json["name"].asString().unwrapOr(data.name);
        else if (json.contains("level_name")) data.name = json["level_name"].asString().unwrapOr(data.name);

        if (json.contains("author")) data.author = json["author"].asString().unwrapOr("");

        // Framerate
        if (json.contains("fps")) {
            data.recordFPS = static_cast<float>(json["fps"].asDouble().unwrapOr(240.0));
        } else if (json.contains("framerate")) {
            data.recordFPS = static_cast<float>(json["framerate"].asDouble().unwrapOr(240.0));
        } else if (json.contains("delta")) {
            double delta = json["delta"].asDouble().unwrapOr(0.0);
            if (delta > 0.0) data.recordFPS = static_cast<float>(1.0 / delta);
        }

        if (json.contains("bot") || json.contains("version")) {
            data.botName = "MegaHack / " + json["version"].asString().unwrapOr("Bot");
        }

        // Locate inputs array
        std::string inputKey = "";
        if (json.contains("inputs") && json["inputs"].isArray()) inputKey = "inputs";
        else if (json.contains("actions") && json["actions"].isArray()) inputKey = "actions";
        else if (json.contains("macro") && json["macro"].isArray()) inputKey = "macro";
        else if (json.contains("events") && json["events"].isArray()) inputKey = "events";
        else if (json.contains("clicks") && json["clicks"].isArray()) inputKey = "clicks";
        else if (json.contains("data") && json["data"].isArray()) inputKey = "data";

        auto parseInputItem = [&](matjson::Value const& inp) {
            MacroInput action;
            action.frame = 0;
            action.isPress = true;
            action.player = 1;
            action.button = 1;

            if (inp.contains("frame")) action.frame = static_cast<int>(inp["frame"].asInt().unwrapOr(0));
            else if (inp.contains("f")) action.frame = static_cast<int>(inp["f"].asInt().unwrapOr(0));
            else if (inp.contains("time")) action.frame = static_cast<int>(inp["time"].asDouble().unwrapOr(0.0) * data.recordFPS);
            else if (inp.contains("t")) action.frame = static_cast<int>(inp["t"].asDouble().unwrapOr(0.0) * data.recordFPS);
            else if (inp.contains("step")) action.frame = static_cast<int>(inp["step"].asInt().unwrapOr(0));

            if (inp.contains("down")) action.isPress = inp["down"].asBool().unwrapOr(true);
            else if (inp.contains("hold")) action.isPress = inp["hold"].asBool().unwrapOr(true);
            else if (inp.contains("press")) action.isPress = inp["press"].asBool().unwrapOr(true);
            else if (inp.contains("action")) action.isPress = (inp["action"].asInt().unwrapOr(1) == 1);
            else if (inp.contains("state")) action.isPress = inp["state"].asBool().unwrapOr(true);

            if (inp.contains("player")) action.player = static_cast<int>(inp["player"].asInt().unwrapOr(1));
            else if (inp.contains("p2")) action.player = inp["p2"].asBool().unwrapOr(false) ? 2 : 1;
            else if (inp.contains("player2")) action.player = inp["player2"].asBool().unwrapOr(false) ? 2 : 1;

            if (inp.contains("button")) action.button = static_cast<int>(inp["button"].asInt().unwrapOr(1));
            else if (inp.contains("btn")) action.button = static_cast<int>(inp["btn"].asInt().unwrapOr(1));

            data.inputs.push_back(action);
        };

        if (!inputKey.empty()) {
            auto& inputs = json[inputKey];
            for (size_t i = 0; i < inputs.size(); i++) {
                parseInputItem(inputs[i]);
            }
        } else if (json.isArray()) {
            for (size_t i = 0; i < json.size(); i++) {
                parseInputItem(json[i]);
            }
        }

        std::sort(data.inputs.begin(), data.inputs.end(),
            [](MacroInput const& a, MacroInput const& b) {
                return a.frame < b.frame;
            }
        );

        data.valid = !data.inputs.empty();
        if (!data.valid) {
            data.error = "No input actions found in JSON macro";
        } else {
            log::info("[Polskie FWC] Loaded JSON macro '{}': {} inputs at {:.0f} FPS",
                data.name, data.inputs.size(), data.recordFPS);
        }

    } catch (std::exception const& e) {
        data.error = std::string("JSON Macro parse error: ") + e.what();
    }

    return data;
}

// ============================================================
// Plain Text / CSV Macro Parser
// ============================================================
MacroParser::MacroData MacroParser::parsePlainText(std::string const& path) {
    MacroData data;
    data.botName = "Plain Text Macro";
    data.name = fs::path(path).stem().string();

    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            data.error = "Could not open text macro: " + path;
            return data;
        }

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#' || line[0] == '/') continue;

            std::stringstream ss(line);
            int frame = 0;
            int press = 1;
            int player = 1;
            int button = 1;

            if (ss >> frame >> press) {
                if (!(ss >> player)) player = 1;
                if (!(ss >> button)) button = 1;

                MacroInput inp;
                inp.frame = frame;
                inp.isPress = (press != 0);
                inp.player = player;
                inp.button = button;
                data.inputs.push_back(inp);
            }
        }

        data.valid = !data.inputs.empty();
        if (!data.valid) {
            data.error = "No valid input lines found in text macro";
        }

    } catch (std::exception const& e) {
        data.error = std::string("Error parsing text macro: ") + e.what();
    }

    return data;
}

std::vector<std::string> MacroParser::findMacroFiles(std::string const& directory) {
    std::vector<std::string> files;
    try {
        if (!fs::exists(directory)) return files;

        for (auto const& entry : fs::directory_iterator(directory)) {
            if (!entry.is_regular_file()) continue;

            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            std::string name = entry.path().filename().string();
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);

            // Skip config/settings files
            if (name == "mod.json" || name == "geode.json" || name == "config.json" || name == "settings.json") {
                continue;
            }

            if (ext == ".gdr2" || ext == ".gdr" || ext == ".mhr" || ext == ".json" || ext == ".txt") {
                files.push_back(entry.path().string());
            }
        }
        std::sort(files.begin(), files.end());
    } catch (...) {}
    return files;
}

std::vector<std::string> MacroParser::getDefaultMacroDirs() {
    std::vector<std::string> dirs;

    auto savePath = Mod::get()->getSaveDir();
    dirs.push_back(savePath.string());

    auto gdSavePath = dirs::getSaveDir();
    dirs.push_back(gdSavePath.string());
    dirs.push_back((gdSavePath / "macros").string());
    dirs.push_back((gdSavePath / "replays").string());

    // Eclipse Menu folder locations
    dirs.push_back((gdSavePath / "geode" / "mods" / "eclipse.eclipse-menu" / "replays").string());
    dirs.push_back((gdSavePath / "geode" / "mods" / "eclipse.eclipse-menu" / "macros").string());

#ifdef GEODE_IS_ANDROID
    dirs.push_back("/sdcard/GeometryDash/eclipse/replays");
    dirs.push_back("/sdcard/GeometryDash/eclipse/macros");
    dirs.push_back("/sdcard/GeometryDash/macros");
    dirs.push_back("/sdcard/GeometryDash/replays");
    dirs.push_back("/storage/emulated/0/GeometryDash/macros");
    dirs.push_back("/storage/emulated/0/GeometryDash/replays");
    dirs.push_back("/storage/emulated/0/Download");
#endif

    return dirs;
}
