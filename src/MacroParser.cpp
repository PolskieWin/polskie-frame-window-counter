#include "MacroParser.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <cstring>
#include <Geode/utils/file.hpp>

namespace fs = std::filesystem;

namespace {
    using Bytes = std::vector<uint8_t>;

    struct ByteReader {
        Bytes const& data;
        size_t pos = 0;
        bool bad = false;

        explicit ByteReader(Bytes const& b, size_t start = 0) : data(b), pos(start) {}

        size_t left() const {
            return (pos < data.size()) ? (data.size() - pos) : 0;
        }

        bool has(size_t n) const {
            return left() >= n;
        }

        bool need(size_t n) {
            if (has(n)) return true;
            bad = true;
            return false;
        }

        void skip(size_t n) {
            if (need(n)) pos += n;
            else pos = data.size();
        }

        uint8_t u8() {
            return need(1) ? data[pos++] : 0;
        }

        template <typename T>
        T le() {
            T val{};
            if (!need(sizeof(T))) return val;
            std::memcpy(&val, data.data() + pos, sizeof(T));
            pos += sizeof(T);
            return val;
        }

        template <typename T>
        T be() {
            T val{};
            if (!need(sizeof(T))) return val;
            uint8_t tmp[sizeof(T)];
            for (size_t i = 0; i < sizeof(T); i++) {
                tmp[i] = data[pos + sizeof(T) - 1 - i];
            }
            std::memcpy(&val, tmp, sizeof(T));
            pos += sizeof(T);
            return val;
        }

        uint64_t varint() {
            uint64_t val = 0;
            for (int shift = 0; shift < 64; shift += 7) {
                if (!need(1)) return val;
                uint8_t b = data[pos++];
                val |= static_cast<uint64_t>(b & 0x7F) << shift;
                if (!(b & 0x80)) return val;
            }
            bad = true;
            return val;
        }

        std::string str(size_t n) {
            if (!need(n)) return {};
            std::string s(reinterpret_cast<const char*>(data.data() + pos), n);
            pos += n;
            return s;
        }
    };

    static Bytes readFileBytes(std::string const& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) return {};
        file.seekg(0, std::ios::end);
        size_t size = static_cast<size_t>(file.tellg());
        file.seekg(0, std::ios::beg);
        Bytes b(size);
        file.read(reinterpret_cast<char*>(b.data()), size);
        return b;
    }

    static std::string readFileText(std::string const& path) {
        auto res = file::readString(fs::path(path));
        if (res) return res.unwrap();
        std::ifstream file(path);
        if (!file.is_open()) return {};
        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }
}

// ============================================================
// Format Detection
// ============================================================
MacroParser::MacroFormat MacroParser::detectFormat(std::string const& path) {
    std::string ext = fs::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    std::string filename = fs::path(path).filename().string();
    std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);

    // Read initial bytes
    std::ifstream file(path, std::ios::binary);
    if (file.is_open()) {
        char buf[16] = {};
        file.read(buf, 16);
        std::streamsize bytesRead = file.gcount();

        // GDR2 magic: "GDR" at offset 0
        if (bytesRead >= 3 && buf[0] == 'G' && buf[1] == 'D' && buf[2] == 'R') {
            return MacroFormat::ECLIPSE_GDR2;
        }

        // MegaHack binary magic: "HACKPRO\x07" (8 bytes)
        static const uint8_t kMhrMagic[8] = {'H', 'A', 'C', 'K', 'P', 'R', 'O', 0x07};
        if (bytesRead >= 8 && std::memcmp(buf, kMhrMagic, 8) == 0) {
            return MacroFormat::MHR_BINARY;
        }

        // ReplayBot magic: "RPLY"
        if (bytesRead >= 4 && buf[0] == 'R' && buf[1] == 'P' && buf[2] == 'L' && buf[3] == 'Y') {
            return MacroFormat::REPLAYBOT;
        }

        // JSON check: begins with '{' or '['
        for (int i = 0; i < bytesRead; i++) {
            char c = buf[i];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') continue;
            if (c == '{' || c == '[') {
                if (filename.ends_with(".mhr.json") || ext == ".mhr") {
                    return MacroFormat::MHR_JSON;
                }
                return MacroFormat::JSON;
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
        case MacroFormat::MHR_BINARY:
            data = parseMHRBinary(path);
            break;
        case MacroFormat::MHR_JSON:
            data = parseMHRJson(path);
            break;
        case MacroFormat::REPLAYBOT:
            data = parseReplayBot(path);
            break;
        case MacroFormat::JSON:
            data = parseJsonMacro(path);
            break;
        case MacroFormat::PLAIN_TEXT:
            data = parsePlainText(path);
            break;
        default:
            data = parseJsonMacro(path);
            break;
    }

    // Cascading fallbacks if primary parser produced no inputs
    if (!data.valid || data.inputs.empty()) {
        if (format != MacroFormat::ECLIPSE_GDR2) {
            auto gdrAttempt = parseEclipseGDR2(path);
            if (gdrAttempt.valid && !gdrAttempt.inputs.empty()) return gdrAttempt;
        }
        if (format != MacroFormat::MHR_BINARY) {
            auto mhrAttempt = parseMHRBinary(path);
            if (mhrAttempt.valid && !mhrAttempt.inputs.empty()) return mhrAttempt;
        }
        if (format != MacroFormat::MHR_JSON && format != MacroFormat::JSON) {
            auto jsonAttempt = parseJsonMacro(path);
            if (jsonAttempt.valid && !jsonAttempt.inputs.empty()) return jsonAttempt;
        }
        if (format != MacroFormat::XDBOT) {
            auto xdAttempt = parseXdBot(path);
            if (xdAttempt.valid && !xdAttempt.inputs.empty()) return xdAttempt;
        }
        if (format != MacroFormat::PLAIN_TEXT) {
            auto txtAttempt = parsePlainText(path);
            if (txtAttempt.valid && !txtAttempt.inputs.empty()) return txtAttempt;
        }
    }

    return data;
}

// ============================================================
// Eclipse Menu (.gdr2 / .gdr) — Binary LEB128 format ("esplice")
// ============================================================
MacroParser::MacroData MacroParser::parseEclipseGDR2(std::string const& path) {
    MacroData data;
    data.botName = "Eclipse Menu (GDR2)";
    data.name = fs::path(path).stem().string();

    try {
        Bytes b = readFileBytes(path);
        if (b.size() < 4) {
            data.error = "File too small for GDR2 header";
            return data;
        }

        // Must start with "GDR"
        if (b[0] != 'G' || b[1] != 'D' || b[2] != 'R') {
            data.error = "Not a GDR2 format (missing GDR magic)";
            return data;
        }

        ByteReader r(b, 3);
        r.varint(); // format version

        auto readVarStr = [&]() -> std::string {
            size_t len = static_cast<size_t>(r.varint());
            return r.str(len);
        };

        std::string inputTag = readVarStr();
        data.author = readVarStr();
        readVarStr(); // description
        r.be<float>(); // duration
        r.varint(); // game version

        double tps = r.be<double>();
        if (tps >= 30.0 && tps <= 2000.0) {
            data.recordFPS = static_cast<float>(tps);
        } else {
            data.recordFPS = 240.0f;
        }

        r.varint(); // seed
        r.varint(); // coins
        r.varint(); // ldm
        bool platformer = (r.varint() != 0);
        data.botName = readVarStr();
        if (data.botName.empty()) data.botName = "Eclipse Menu";
        r.varint(); // bot version
        r.varint(); // level id
        data.name = readVarStr();
        if (data.name.empty()) data.name = fs::path(path).stem().string();

        r.skip(static_cast<size_t>(r.varint())); // skip extension blob

        uint64_t deaths = r.varint();
        for (uint64_t d = 0; d < deaths && !r.bad; d++) {
            r.varint();
        }

        r.varint(); // total inputs
        uint64_t p1Left = r.varint();

        if (r.bad) {
            data.error = "Corrupted or cut-off GDR2 header";
            return data;
        }

        int64_t prev = 0;
        while (r.left() > 0 && !r.bad) {
            uint64_t packed = r.varint();
            if (r.bad) break;

            MacroInput inp;
            if (platformer) {
                inp.frame = static_cast<int>(prev + static_cast<int64_t>(packed >> 3));
                inp.button = static_cast<int>((packed >> 1) & 3);
                if (inp.button == 0) inp.button = 1;
            } else {
                inp.frame = static_cast<int>(prev + static_cast<int64_t>(packed >> 1));
                inp.button = 1;
            }

            inp.isPress = (packed & 1) != 0;
            inp.player = (p1Left == 0) ? 2 : 1;

            data.inputs.push_back(inp);

            if (!inputTag.empty()) {
                r.skip(static_cast<size_t>(r.varint()));
            }

            prev = inp.frame;
            if (p1Left > 0 && --p1Left == 0) {
                prev = 0;
            }
        }

        data.valid = !data.inputs.empty();
        if (data.valid) {
            log::info("[Polskie FWC] Loaded Eclipse GDR2 macro '{}': {} inputs at {:.0f} FPS",
                data.name, data.inputs.size(), data.recordFPS);
        } else {
            data.error = "No input actions could be read from Eclipse GDR2";
        }

    } catch (std::exception const& e) {
        data.error = std::string("Error reading Eclipse GDR2: ") + e.what();
    }

    return data;
}

// ============================================================
// MegaHack Replay Binary (.mhr) — 'HACKPRO\x07' format
// ============================================================
MacroParser::MacroData MacroParser::parseMHRBinary(std::string const& path) {
    MacroData data;
    data.botName = "MegaHack Replay (Binary)";
    data.name = fs::path(path).stem().string();

    try {
        Bytes b = readFileBytes(path);
        if (b.size() < 24) {
            data.error = "File too small for MegaHack binary replay";
            return data;
        }

        static const uint8_t kMhrMagic[8] = {'H', 'A', 'C', 'K', 'P', 'R', 'O', 0x07};
        if (std::memcmp(b.data(), kMhrMagic, 8) != 0) {
            data.error = "Missing MegaHack 'HACKPRO' binary magic";
            return data;
        }

        ByteReader r(b, 8);
        int32_t metaSize = r.le<int32_t>();
        int32_t tps = r.le<int32_t>();
        if (r.bad || metaSize < 4) {
            data.error = "Corrupted MegaHack replay header";
            return data;
        }

        data.recordFPS = (tps >= 30 && tps <= 2000) ? static_cast<float>(tps) : 240.0f;

        // Skip to event records
        r.pos = 16 + static_cast<size_t>(metaSize - 4) + 8;
        uint32_t eventSize = r.le<uint32_t>();
        uint32_t count = r.le<uint32_t>();

        if (r.bad || eventSize < 8) {
            data.error = "Corrupted MegaHack events header";
            return data;
        }

        for (uint32_t k = 0; k < count && r.has(eventSize); k++) {
            size_t at = r.pos;
            r.skip(2);
            bool down = (r.u8() == 1);
            bool p2 = (r.u8() == 1);
            int32_t frame = r.le<int32_t>();
            r.pos = at + eventSize;

            MacroInput inp;
            inp.frame = frame;
            inp.isPress = down;
            inp.player = p2 ? 2 : 1;
            inp.button = 1;

            data.inputs.push_back(inp);
        }

        data.valid = !data.inputs.empty();
        if (data.valid) {
            log::info("[Polskie FWC] Loaded MegaHack binary macro '{}': {} inputs at {:.0f} FPS",
                data.name, data.inputs.size(), data.recordFPS);
        } else {
            data.error = "No input actions found in MegaHack .mhr binary file";
        }

    } catch (std::exception const& e) {
        data.error = std::string("Error parsing MegaHack binary: ") + e.what();
    }

    return data;
}

// ============================================================
// MegaHack Replay JSON (.mhr.json or .mhr with JSON content)
// ============================================================
MacroParser::MacroData MacroParser::parseMHRJson(std::string const& path) {
    MacroData data;
    data.botName = "MegaHack Replay (JSON)";
    data.name = fs::path(path).stem().string();

    try {
        std::string content = readFileText(path);
        auto jsonRes = matjson::parse(content);
        if (!jsonRes) {
            data.error = "Invalid JSON syntax in MegaHack JSON: " + jsonRes.unwrapErr().message;
            return data;
        }
        auto json = jsonRes.unwrap();

        if (json.contains("meta") && json["meta"].isObject()) {
            auto const& meta = json["meta"];
            if (meta.contains("fps")) {
                data.recordFPS = static_cast<float>(meta["fps"].asDouble().unwrapOr(240.0));
            }
            if (meta.contains("name")) {
                data.name = meta["name"].asString().unwrapOr(data.name);
            }
            if (meta.contains("bot")) {
                data.botName = meta["bot"].asString().unwrapOr("MegaHack");
            }
        }

        if (json.contains("events") && json["events"].isArray()) {
            auto const& events = json["events"];
            for (size_t i = 0; i < events.size(); i++) {
                auto const& ev = events[i];
                MacroInput inp;
                inp.frame = static_cast<int>(ev["frame"].asInt().unwrapOr(0));
                inp.isPress = ev["down"].asBool().unwrapOr(true);
                inp.player = ev["p2"].asBool().unwrapOr(false) ? 2 : 1;
                inp.button = static_cast<int>(ev["button"].asInt().unwrapOr(1));
                data.inputs.push_back(inp);
            }
        }

        data.valid = !data.inputs.empty();
        if (data.valid) {
            log::info("[Polskie FWC] Loaded MegaHack JSON macro '{}': {} inputs at {:.0f} FPS",
                data.name, data.inputs.size(), data.recordFPS);
        } else {
            data.error = "No events found in MegaHack JSON macro";
        }

    } catch (std::exception const& e) {
        data.error = std::string("Error parsing MegaHack JSON: ") + e.what();
    }

    return data;
}

// ============================================================
// xdBot (.txt / .csv / .json)
// ============================================================
MacroParser::MacroData MacroParser::parseXdBot(std::string const& path) {
    MacroData data;
    data.botName = "xdBot";
    data.name = fs::path(path).stem().string();

    try {
        std::string content = readFileText(path);
        std::stringstream ss(content);
        std::string line;

        // Line 0: FPS
        if (std::getline(ss, line)) {
            try {
                float fps = std::stof(line);
                if (fps >= 30.0f && fps <= 2000.0f) data.recordFPS = fps;
            } catch (...) {}
        }

        while (std::getline(ss, line)) {
            if (line.empty() || line[0] == '#') continue;
            std::stringstream lineStream(line);
            std::string part;
            std::vector<std::string> parts;
            while (std::getline(lineStream, part, '|')) {
                parts.push_back(part);
            }

            if (parts.size() >= 2) {
                MacroInput inp;
                inp.frame = std::stoi(parts[0]);
                inp.isPress = (parts[1] == "1" || parts[1] == "true");
                inp.button = 1;
                inp.player = (parts.size() >= 4 && parts[3] == "0") ? 2 : 1;
                data.inputs.push_back(inp);
            }
        }

        data.valid = !data.inputs.empty();
    } catch (std::exception const& e) {
        data.error = e.what();
    }

    return data;
}

// ============================================================
// xBot (.txt / .csv)
// ============================================================
MacroParser::MacroData MacroParser::parseXbot(std::string const& path) {
    MacroData data;
    data.botName = "xBot";
    data.name = fs::path(path).stem().string();

    try {
        std::string content = readFileText(path);
        std::stringstream ss(content);
        std::string line;

        while (std::getline(ss, line)) {
            if (line.rfind("fps:", 0) == 0) {
                try {
                    data.recordFPS = std::stof(line.substr(4));
                } catch (...) {}
                continue;
            }
            if (line == "frames" || line.empty()) continue;

            std::stringstream lineStream(line);
            int state = 0;
            int frame = 0;
            if (lineStream >> state >> frame) {
                MacroInput inp;
                inp.frame = frame;
                inp.isPress = (state % 2 == 1);
                inp.player = (state > 1) ? 2 : 1;
                inp.button = 1;
                data.inputs.push_back(inp);
            }
        }

        data.valid = !data.inputs.empty();
    } catch (std::exception const& e) {
        data.error = e.what();
    }

    return data;
}

// ============================================================
// TASBot (.json)
// ============================================================
MacroParser::MacroData MacroParser::parseTasBot(std::string const& path) {
    MacroData data;
    data.botName = "TASBot";
    data.name = fs::path(path).stem().string();

    try {
        std::string content = readFileText(path);
        auto jsonRes = matjson::parse(content);
        if (!jsonRes) return data;
        auto json = jsonRes.unwrap();

        if (json.contains("fps")) {
            data.recordFPS = static_cast<float>(json["fps"].asDouble().unwrapOr(240.0));
        }

        if (json.contains("macro") && json["macro"].isArray()) {
            auto const& rows = json["macro"];
            for (size_t i = 0; i < rows.size(); i++) {
                auto const& row = rows[i];
                int frame = static_cast<int>(row["frame"].asInt().unwrapOr(0));

                for (int pl = 0; pl < 2; pl++) {
                    std::string key = (pl == 1) ? "player_2" : "player_1";
                    if (row.contains(key) && row[key].isObject()) {
                        auto const& p = row[key];
                        if (p.contains("click")) {
                            int click = static_cast<int>(p["click"].asInt().unwrapOr(0));
                            if (click != 0) {
                                MacroInput inp;
                                inp.frame = frame;
                                inp.isPress = (click == 1);
                                inp.player = pl + 1;
                                inp.button = 1;
                                data.inputs.push_back(inp);
                            }
                        }
                    }
                }
            }
        }

        data.valid = !data.inputs.empty();
    } catch (...) {}

    return data;
}

// ============================================================
// ReplayBot (.replay, .rply)
// ============================================================
MacroParser::MacroData MacroParser::parseReplayBot(std::string const& path) {
    MacroData data;
    data.botName = "ReplayBot";
    data.name = fs::path(path).stem().string();

    try {
        Bytes b = readFileBytes(path);
        if (b.size() < 10 || b[0] != 'R' || b[1] != 'P' || b[2] != 'L' || b[3] != 'Y') {
            data.error = "Not a valid ReplayBot file";
            return data;
        }

        ByteReader r(b, 6);
        data.recordFPS = r.le<float>();

        while (r.has(5)) {
            uint32_t frame = r.le<uint32_t>();
            uint8_t st = r.u8();

            MacroInput inp;
            inp.frame = static_cast<int>(frame);
            inp.isPress = (st & 1) != 0;
            inp.player = (st & 2) ? 2 : 1;
            inp.button = 1;
            data.inputs.push_back(inp);
        }

        data.valid = !data.inputs.empty();
    } catch (std::exception const& e) {
        data.error = e.what();
    }

    return data;
}

// ============================================================
// Generic JSON Macro Parser (handles any replay format)
// ============================================================
MacroParser::MacroData MacroParser::parseJsonMacro(std::string const& path) {
    MacroData data;
    data.botName = "JSON Bot Macro";
    data.name = fs::path(path).stem().string();

    try {
        std::string content = readFileText(path);
        auto jsonRes = matjson::parse(content);
        if (!jsonRes) {
            data.error = "Invalid JSON syntax: " + jsonRes.unwrapErr().message;
            return data;
        }
        auto json = jsonRes.unwrap();

        // Check for metadata
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

        if (json.contains("bot")) {
            data.botName = json["bot"].asString().unwrapOr("Bot");
        }

        // Locate input array
        std::string inputKey = "";
        if (json.contains("inputs") && json["inputs"].isArray()) inputKey = "inputs";
        else if (json.contains("events") && json["events"].isArray()) inputKey = "events";
        else if (json.contains("actions") && json["actions"].isArray()) inputKey = "actions";
        else if (json.contains("macro") && json["macro"].isArray()) inputKey = "macro";
        else if (json.contains("clicks") && json["clicks"].isArray()) inputKey = "clicks";
        else if (json.contains("data") && json["data"].isArray()) inputKey = "data";
        else if (json.contains("Echo Replay") && json["Echo Replay"].isArray()) inputKey = "Echo Replay";

        auto parseInputItem = [&](matjson::Value const& inp) {
            MacroInput action;
            action.frame = 0;
            action.isPress = true;
            action.player = 1;
            action.button = 1;

            if (inp.contains("frame")) action.frame = static_cast<int>(inp["frame"].asInt().unwrapOr(0));
            else if (inp.contains("Frame")) action.frame = static_cast<int>(inp["Frame"].asInt().unwrapOr(0));
            else if (inp.contains("f")) action.frame = static_cast<int>(inp["f"].asInt().unwrapOr(0));
            else if (inp.contains("time")) action.frame = static_cast<int>(inp["time"].asDouble().unwrapOr(0.0) * data.recordFPS);
            else if (inp.contains("step")) action.frame = static_cast<int>(inp["step"].asInt().unwrapOr(0));

            if (inp.contains("down")) action.isPress = inp["down"].asBool().unwrapOr(true);
            else if (inp.contains("hold")) action.isPress = inp["hold"].asBool().unwrapOr(true);
            else if (inp.contains("Hold")) action.isPress = inp["Hold"].asBool().unwrapOr(true);
            else if (inp.contains("press")) action.isPress = inp["press"].asBool().unwrapOr(true);
            else if (inp.contains("action")) action.isPress = (inp["action"].asInt().unwrapOr(1) == 1);
            else if (inp.contains("state")) action.isPress = inp["state"].asBool().unwrapOr(true);

            if (inp.contains("player")) action.player = static_cast<int>(inp["player"].asInt().unwrapOr(1));
            else if (inp.contains("p2")) action.player = inp["p2"].asBool().unwrapOr(false) ? 2 : 1;
            else if (inp.contains("player2")) action.player = inp["player2"].asBool().unwrapOr(false) ? 2 : 1;
            else if (inp.contains("Player 2")) action.player = inp["Player 2"].asBool().unwrapOr(false) ? 2 : 1;

            if (inp.contains("button")) action.button = static_cast<int>(inp["button"].asInt().unwrapOr(1));
            else if (inp.contains("btn")) action.button = static_cast<int>(inp["btn"].asInt().unwrapOr(1));

            data.inputs.push_back(action);
        };

        if (!inputKey.empty()) {
            auto const& list = json[inputKey];
            for (size_t i = 0; i < list.size(); i++) {
                parseInputItem(list[i]);
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
        if (data.valid) {
            log::info("[Polskie FWC] Loaded JSON macro '{}': {} inputs at {:.0f} FPS",
                data.name, data.inputs.size(), data.recordFPS);
        } else {
            data.error = "No recognized input actions found in JSON macro";
        }

    } catch (std::exception const& e) {
        data.error = std::string("JSON parse error: ") + e.what();
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
        std::string content = readFileText(path);
        std::stringstream ss(content);
        std::string line;

        while (std::getline(ss, line)) {
            if (line.empty() || line[0] == '#' || line[0] == '/') continue;

            std::stringstream lineStream(line);
            int frame = 0;
            int press = 1;
            int player = 1;
            int button = 1;

            if (lineStream >> frame >> press) {
                if (!(lineStream >> player)) player = 1;
                if (!(lineStream >> button)) button = 1;

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
            data.error = "No valid lines parsed from text macro";
        }

    } catch (std::exception const& e) {
        data.error = std::string("Text macro parse error: ") + e.what();
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

            if (ext == ".gdr2" || ext == ".gdr" || ext == ".mhr" || ext == ".json" || ext == ".txt" || ext == ".csv" || ext == ".rply") {
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
