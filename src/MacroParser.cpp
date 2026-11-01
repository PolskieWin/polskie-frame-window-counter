#include "MacroParser.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <cstring>
#include <cctype>
#include <cmath>
#include <initializer_list>
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
        auto res = file::readString(path);
        if (res) return res.unwrap();
        std::ifstream file(path);
        if (!file.is_open()) return {};
        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

    using Json = matjson::Value;

    static std::string lower(std::string value) {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        return value;
    }

    static bool numberValue(Json const& value, double& out) {
        auto asDouble = value.asDouble();
        if (asDouble) {
            out = asDouble.unwrap();
            return std::isfinite(out);
        }
        auto asInteger = value.asInt();
        if (asInteger) {
            out = static_cast<double>(asInteger.unwrap());
            return true;
        }
        return false;
    }

    static bool numberField(Json const& object, std::initializer_list<char const*> keys, double& out) {
        for (auto key : keys) {
            if (numberValue(object[key], out)) return true;
        }
        return false;
    }

    static std::string stringField(Json const& object, std::initializer_list<char const*> keys) {
        for (auto key : keys) {
            auto value = object[key].asString();
            if (value) return value.unwrap();
        }
        return {};
    }

    static bool stateValue(Json const& value, bool& isPress) {
        auto asBool = value.asBool();
        if (asBool) {
            isPress = asBool.unwrap();
            return true;
        }

        auto asString = value.asString();
        if (asString) {
            auto state = lower(asString.unwrap());
            if (state == "press" || state == "pressed" || state == "down" || state == "1") {
                isPress = true;
                return true;
            }
            if (state == "release" || state == "released" || state == "up" || state == "0") {
                isPress = false;
                return true;
            }
        }

        double numeric = 0.0;
        if (numberValue(value, numeric) && (numeric == 0.0 || numeric == 1.0)) {
            isPress = numeric == 1.0;
            return true;
        }
        return false;
    }

    static bool eventFromJson(Json const& value, MacroParser::MacroInput& input) {
        auto row = value.asArray();
        if (row) {
            auto values = row.unwrap();
            if (values.size() < 2 || !numberValue(values[0], input.timeSeconds) ||
                !stateValue(values[1], input.isPress)) {
                return false;
            }
            input.frame = static_cast<int>(std::floor(input.timeSeconds));
            input.subFrame = input.timeSeconds - std::floor(input.timeSeconds);
            input.hasTimeSeconds = false;
            if (values.size() > 2) {
                double player = 1.0;
                if (numberValue(values[2], player)) input.player = (player == 2.0) ? 2 : 1;
            }
            if (values.size() > 3) {
                double button = 1.0;
                if (numberValue(values[3], button)) input.button = static_cast<int>(button);
            }
            return true;
        }

        double frame = 0.0;
        bool hasFrame = numberField(value, {"frame", "frames", "tick", "ticks", "frameIndex", "frame_index"}, frame);
        double seconds = 0.0;
        bool hasSeconds = numberField(value, {"timeSeconds", "time_seconds", "seconds"}, seconds);
        if (!hasSeconds) hasSeconds = numberField(value, {"timestamp", "timestampSeconds", "timestamp_seconds"}, seconds);
        double milliseconds = 0.0;
        bool hasMilliseconds = numberField(value, {"timeMs", "time_ms", "timestampMs", "timestamp_ms"}, milliseconds);

        if (!hasFrame && !hasSeconds && !hasMilliseconds) {
            if (numberField(value, {"time", "t"}, frame)) {
                hasFrame = true;
            }
        }
        if (!hasFrame && !hasSeconds && !hasMilliseconds) return false;

        bool hasState = false;
        for (auto key : {"isPress", "is_pressed", "down", "pressed", "press", "state", "action", "type"}) {
            if (stateValue(value[key], input.isPress)) {
                hasState = true;
                break;
            }
        }
        if (!hasState) return false;

        double player = 1.0;
        if (numberField(value, {"player", "playerIndex", "player_index", "playerNum", "player_num"}, player)) {
            input.player = (player == 2.0 || player == 1.0) ? static_cast<int>(player) : (player == 0.0 ? 1 : 2);
        }
        double button = 1.0;
        if (numberField(value, {"button", "buttonId", "button_id", "key"}, button)) {
            input.button = static_cast<int>(button);
        } else {
            auto buttonName = lower(stringField(value, {"button", "key"}));
            if (buttonName == "left") input.button = 2;
            else if (buttonName == "right") input.button = 3;
        }

        double subFrame = 0.0;
        if (numberField(value, {"subTick", "subtick", "sub_frame", "subFrame", "fraction"}, subFrame)) {
            frame += subFrame;
        }

        if (hasFrame) {
            input.frame = static_cast<int>(std::floor(frame));
            input.subFrame = frame - std::floor(frame);
        }
        if (hasSeconds) {
            input.timeSeconds = seconds;
            input.hasTimeSeconds = true;
        } else if (hasMilliseconds) {
            input.timeSeconds = milliseconds / 1000.0;
            input.hasTimeSeconds = true;
        }
        return true;
    }

    static void collectEvents(Json const& value, std::vector<MacroParser::MacroInput>& inputs, int depth = 0) {
        if (depth > 12) return;
        auto array = value.asArray();
        if (array) {
            for (auto const& item : array.unwrap()) {
                MacroParser::MacroInput input;
                if (eventFromJson(item, input)) inputs.push_back(input);
                else collectEvents(item, inputs, depth + 1);
            }
            return;
        }

        for (auto key : {"events", "inputs", "actions", "clicks", "records", "timeline", "replay", "data", "keyframes"}) {
            collectEvents(value[key], inputs, depth + 1);
        }
    }

    static MacroParser::MacroData parseEventFile(std::string const& path, std::string const& formatName) {
        MacroParser::MacroData data;
        data.name = fs::path(path).filename().string();
        auto text = readFileText(path);
        auto parsed = matjson::parse(text);
        if (!parsed) {
            data.error = formatName + " binary/structured payload is not a supported JSON event stream";
            return data;
        }

        auto root = parsed.unwrap();
        auto metadata = root["metadata"];
        if (metadata.isNull()) metadata = root["meta"];
        data.author = stringField(root, {"author", "creator"});
        if (data.author.empty()) data.author = stringField(metadata, {"author", "creator"});
        data.botName = stringField(root, {"bot", "botName", "bot_name", "format"});
        if (data.botName.empty()) data.botName = formatName;
        auto name = stringField(root, {"name", "levelName", "level_name"});
        if (!name.empty()) data.name = name;

        double fps = 0.0;
        if (!numberField(root, {"recordFPS", "record_fps", "fps", "tickRate", "tick_rate"}, fps)) {
            numberField(metadata, {"recordFPS", "record_fps", "fps", "tickRate", "tick_rate"}, fps);
        }
        if (fps > 0.0 && fps <= 10000.0) data.recordFPS = static_cast<float>(fps);

        collectEvents(root, data.inputs);
        auto eventTime = [&data](MacroParser::MacroInput const& input) {
            if (input.hasTimeSeconds) return input.timeSeconds;
            return (input.frame + input.subFrame) / data.recordFPS;
        };
        std::stable_sort(data.inputs.begin(), data.inputs.end(), [&eventTime](auto const& a, auto const& b) {
            return eventTime(a) < eventTime(b);
        });
        for (auto& input : data.inputs) {
            if (!input.hasTimeSeconds) {
                input.timeSeconds = (input.frame + input.subFrame) / data.recordFPS;
                input.hasTimeSeconds = true;
            }
        }

        data.valid = !data.inputs.empty();
        if (!data.valid) data.error = "No timed press/release events were found";
        return data;
    }
}

// ============================================================
// Format Detection
// ============================================================
MacroParser::MacroFormat MacroParser::detectFormat(std::string const& path) {
    auto ext = lower(fs::path(path).extension().string());
    if (ext == ".gdr2" || ext == ".gdr") {
        return ext == ".gdr2" ? MacroFormat::ECLIPSE_GDR2 : MacroFormat::GDREPLAY;
    }
    if (ext == ".slc") return MacroFormat::SILICO;
    if (ext == ".tcm") return MacroFormat::TASBOT_TCM;
    if (ext == ".ybot") return MacroFormat::YBOT;

    auto filename = lower(fs::path(path).filename().string());
    if (filename.ends_with(".mhr.json")) {
        return MacroFormat::MHR_JSON;
    }
    if (ext == ".mhr") {
        return MacroFormat::MHR_BINARY;
    }
    if (ext == ".rply" || ext == ".replay") return MacroFormat::REPLAYBOT;
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
        case MacroFormat::GDREPLAY:
            data = parseGDReplay(path);
            break;
        case MacroFormat::SILICO:
            data = parseSilico(path);
            break;
        case MacroFormat::TASBOT_TCM:
            data = parseTasBotTCM(path);
            break;
        case MacroFormat::YBOT:
            data = parseYBot(path);
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

    return data;
}

MacroParser::MacroData MacroParser::parseEventJson(std::string const& path) {
    return parseEventFile(path, "Replay");
}

MacroParser::MacroData MacroParser::parseGDReplay(std::string const& path) {
    return parseEventFile(path, "GDReplay");
}

MacroParser::MacroData MacroParser::parseSilico(std::string const& path) {
    return parseEventFile(path, "Silico");
}

MacroParser::MacroData MacroParser::parseTasBotTCM(std::string const& path) {
    return parseEventFile(path, "TasBot TCM");
}

MacroParser::MacroData MacroParser::parseYBot(std::string const& path) {
    return parseEventFile(path, "yBot / zBot");
}

MacroParser::MacroData MacroParser::parseEclipseGDR2(std::string const& path) {
    return parseEventFile(path, "Eclipse GDR2");
}

MacroParser::MacroData MacroParser::parseMHRBinary(std::string const& path) {
    MacroData data;
    data.name = fs::path(path).filename().string();
    data.error = "MegaHack binary replay records are not supported by this parser";
    return data;
}

MacroParser::MacroData MacroParser::parseMHRJson(std::string const& path) {
    return parseEventFile(path, "MegaHack JSON");
}

MacroParser::MacroData MacroParser::parseXdBot(std::string const& path) {
    return parseEventFile(path, "xdBot");
}

MacroParser::MacroData MacroParser::parseXbot(std::string const& path) {
    return parseEventFile(path, "xBot");
}

MacroParser::MacroData MacroParser::parseTasBot(std::string const& path) {
    return parseEventFile(path, "TASBot JSON");
}

MacroParser::MacroData MacroParser::parseReplayBot(std::string const& path) {
    return parseEventFile(path, "ReplayBot");
}

MacroParser::MacroData MacroParser::parseJsonMacro(std::string const& path) {
    return parseEventJson(path);
}

MacroParser::MacroData MacroParser::parsePlainText(std::string const& path) {
    MacroData data;
    data.name = fs::path(path).filename().string();
    std::ifstream input(path);
    std::string line;
    while (std::getline(input, line)) {
        for (auto& ch : line) {
            if (ch == ',' || ch == ';' || ch == '\t') ch = ' ';
        }
        std::istringstream row(line);
        double frame = 0.0;
        std::string state;
        int player = 1;
        int button = 1;
        if (!(row >> frame >> state)) continue;
        MacroInput event;
        auto normalizedState = lower(state);
        if (normalizedState == "press" || normalizedState == "pressed" || normalizedState == "down" || normalizedState == "1") {
            event.isPress = true;
        } else if (normalizedState == "release" || normalizedState == "released" || normalizedState == "up" || normalizedState == "0") {
            event.isPress = false;
        } else {
            continue;
        }
        row >> player >> button;
        event.frame = static_cast<int>(std::floor(frame));
        event.subFrame = frame - std::floor(frame);
        event.player = (player == 2) ? 2 : 1;
        event.button = button;
        event.timeSeconds = (event.frame + event.subFrame) / data.recordFPS;
        event.hasTimeSeconds = true;
        data.inputs.push_back(event);
    }
    data.valid = !data.inputs.empty();
    if (!data.valid) data.error = "No valid frame,state[,player,button] rows were found";
    return data;
}

std::vector<std::string> MacroParser::findMacroFiles(std::string const& directory) {
    static std::vector<std::string> const extensions = {
        ".gdr", ".gdr2", ".slc", ".tcm", ".ybot", ".mhr", ".json", ".txt", ".csv", ".rply", ".replay"
    };
    std::vector<std::string> paths;
    std::error_code error;
    if (!fs::is_directory(directory, error)) return paths;
    for (fs::directory_iterator it(directory, error), end; !error && it != end; it.increment(error)) {
        if (!it->is_regular_file(error)) continue;
        auto ext = lower(it->path().extension().string());
        if (std::find(extensions.begin(), extensions.end(), ext) != extensions.end()) {
            paths.push_back(it->path().string());
        }
    }
    std::sort(paths.begin(), paths.end());
    return paths;
}

std::vector<std::string> MacroParser::getDefaultMacroDirs() {
    auto saveDir = fs::path(Mod::get()->getSaveDir().string());
    std::vector<std::string> dirs = {
        (saveDir / "macros").string(),
        (saveDir / "replays").string(),
        saveDir.string()
    };
#ifdef GEODE_IS_ANDROID
    dirs.push_back("/sdcard/GeometryDash/macros");
    dirs.push_back("/storage/emulated/0/Download");
#endif
    return dirs;
}
