#include "MacroParser.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <cstring>
#include <cctype>
#include <cmath>
#include <initializer_list>
#include <limits>
#include <optional>
#include <Geode/utils/file.hpp>
#include <nlohmann/json.hpp>

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
            for (unsigned int shift = 0; shift < 64; shift += 7) {
                if (!need(1)) return val;
                uint8_t b = data[pos++];
                if (shift == 63 && (b & 0xFE) != 0) {
                    bad = true;
                    return 0;
                }
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
        bool hasFrame = numberField(value, {"frame", "frames", "tick", "ticks", "frameIndex", "frame_index", "Frame", "f", "step"}, frame);
        double seconds = 0.0;
        bool hasSeconds = numberField(value, {"timeSeconds", "time_seconds", "seconds", "time", "t", "timestamp", "timestampSeconds", "timestamp_seconds"}, seconds);
        double milliseconds = 0.0;
        bool hasMilliseconds = numberField(value, {"timeMs", "time_ms", "timestampMs", "timestamp_ms"}, milliseconds);

        if (!hasFrame && !hasSeconds && !hasMilliseconds) return false;

        bool hasState = false;
        for (auto key : {"isPress", "is_pressed", "down", "pressed", "press", "hold", "Hold", "state", "action", "type", "p", "d", "holding"}) {
            if (stateValue(value[key], input.isPress)) {
                hasState = true;
                break;
            }
        }
        if (!hasState) return false;

        double player = 1.0;
        if (numberField(value, {"player", "playerIndex", "player_index", "playerNum", "player_num"}, player)) {
            input.player = (player == 2.0 || player == 1.0) ? static_cast<int>(player) : (player == 0.0 ? 1 : 2);
        } else {
            for (auto key : {"2p", "p2", "player2", "Player 2"}) {
                auto playerTwo = value[key].asBool();
                if (playerTwo && playerTwo.unwrap()) {
                    input.player = 2;
                    break;
                }
            }
        }
        double button = 1.0;
        if (numberField(value, {"button", "buttonId", "button_id", "key", "btn"}, button)) {
            input.button = static_cast<int>(button);
        } else {
            auto buttonName = lower(stringField(value, {"button", "key", "btn"}));
            if (buttonName == "left") input.button = 2;
            else if (buttonName == "right") input.button = 3;
            else if (buttonName == "jump" || buttonName == "click" || buttonName == "primary") input.button = 1;
        }

        double windowFrames = 0.0;
        if (numberField(value, {"windowFrames", "window_frames", "marginFrames", "margin_frames"}, windowFrames) && windowFrames >= 0.0) {
            input.windowFrames = windowFrames;
            input.hasWindowFrames = true;
        }

        double subFrame = 0.0;
        if (numberField(value, {"subTick", "subtick", "sub_frame", "subFrame", "fraction"}, subFrame)) {
            frame += subFrame;
        }

        if (hasSeconds) {
            input.timeSeconds = seconds;
            input.hasTimeSeconds = true;
        } else if (hasMilliseconds) {
            input.timeSeconds = milliseconds / 1000.0;
            input.hasTimeSeconds = true;
        }

        if (hasFrame) {
            input.frame = static_cast<int>(std::floor(frame));
            input.subFrame = frame - std::floor(frame);
        } else if (input.hasTimeSeconds) {
            double estFrame = input.timeSeconds * 240.0;
            input.frame = static_cast<int>(std::floor(estFrame));
            input.subFrame = estFrame - std::floor(estFrame);
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

        for (auto key : {"events", "inputs", "actions", "clicks", "records", "timeline", "replay", "data", "keyframes", "macro", "Echo Replay"}) {
            collectEvents(value[key], inputs, depth + 1);
        }
    }

    static MacroParser::MacroData parseEventFile(std::string const& path, std::string const& formatName) {
        MacroParser::MacroData data;
        data.name = fs::path(path).filename().string();
        auto text = readFileText(path);
        std::optional<Json> parsedRoot;
        auto parsedText = matjson::parse(text);
        if (parsedText) {
            parsedRoot = parsedText.unwrap();
        } else {
            auto bytes = readFileBytes(path);
            try {
                auto messagePack = nlohmann::json::from_msgpack(bytes.begin(), bytes.end(), true, false);
                if (!messagePack.is_discarded()) {
                    auto converted = matjson::parse(messagePack.dump());
                    if (converted) parsedRoot = converted.unwrap();
                }
            } catch (...) {}
        }
        if (!parsedRoot) {
            data.error = formatName + " file is not valid JSON or MessagePack";
            return data;
        }

        auto const& root = *parsedRoot;
        auto metadata = root["metadata"];
        if (metadata.isNull()) metadata = root["meta"];
        data.author = stringField(root, {"author", "creator"});
        if (data.author.empty()) data.author = stringField(metadata, {"author", "creator"});
        data.botName = stringField(root, {"bot", "botName", "bot_name", "format"});
        if (data.botName.empty()) data.botName = formatName;
        auto name = stringField(root, {"name", "levelName", "level_name"});
        if (!name.empty()) data.name = name;

        double fps = 0.0;
        if (!numberField(root, {"recordFPS", "record_fps", "fps", "framerate", "tickRate", "tick_rate"}, fps)) {
            numberField(metadata, {"recordFPS", "record_fps", "fps", "framerate", "tickRate", "tick_rate"}, fps);
        }
        if (fps <= 0.0) {
            double frameDelta = 0.0;
            if (numberField(root, {"delta", "frameDelta", "frame_delta"}, frameDelta) && frameDelta > 0.0) {
                fps = 1.0 / frameDelta;
            }
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
    auto filename = lower(fs::path(path).filename().string());
    if (filename.ends_with(".mhr.json")) {
        return MacroFormat::MHR_JSON;
    }

    auto bytes = readFileBytes(path);
    static const uint8_t mhrMagic[] = { 'H', 'A', 'C', 'K', 'P', 'R', 'O', 0x07 };
    if (bytes.size() >= 3 && bytes[0] == 'G' && bytes[1] == 'D' && bytes[2] == 'R') {
        return MacroFormat::ECLIPSE_GDR2;
    }
    if (bytes.size() >= sizeof(mhrMagic) && std::memcmp(bytes.data(), mhrMagic, sizeof(mhrMagic)) == 0) {
        return MacroFormat::MHR_BINARY;
    }
    if (bytes.size() >= 4 && bytes[0] == 'R' && bytes[1] == 'P' && bytes[2] == 'L' && bytes[3] == 'Y') {
        return MacroFormat::REPLAYBOT;
    }

    if (ext == ".gdr2") return MacroFormat::ECLIPSE_GDR2;
    if (ext == ".gdr") return MacroFormat::GDREPLAY;
    if (ext == ".slc") return MacroFormat::SILICO;
    if (ext == ".tcm") return MacroFormat::TASBOT_TCM;
    if (ext == ".ybot") return MacroFormat::YBOT;
    if (ext == ".mhr") {
        auto first = std::find_if(bytes.begin(), bytes.end(), [](uint8_t ch) {
            return !std::isspace(static_cast<unsigned char>(ch));
        });
        if (first != bytes.end() && (*first == '{' || *first == '[')) return MacroFormat::MHR_JSON;
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

    // Multi-stage fallback: try every available decoder if initial format parser failed
    if (!data.valid || data.inputs.empty()) {
        auto gdr = parseEclipseGDR2(path);
        if (gdr.valid && !gdr.inputs.empty()) return gdr;

        auto mhrBin = parseMHRBinary(path);
        if (mhrBin.valid && !mhrBin.inputs.empty()) return mhrBin;

        auto mhrJson = parseMHRJson(path);
        if (mhrJson.valid && !mhrJson.inputs.empty()) return mhrJson;

        auto rply = parseReplayBot(path);
        if (rply.valid && !rply.inputs.empty()) return rply;

        auto jsonMacro = parseJsonMacro(path);
        if (jsonMacro.valid && !jsonMacro.inputs.empty()) return jsonMacro;

        auto tasBot = parseTasBot(path);
        if (tasBot.valid && !tasBot.inputs.empty()) return tasBot;

        auto xdBot = parseXdBot(path);
        if (xdBot.valid && !xdBot.inputs.empty()) return xdBot;

        auto xBot = parseXbot(path);
        if (xBot.valid && !xBot.inputs.empty()) return xBot;

        auto plain = parsePlainText(path);
        if (plain.valid && !plain.inputs.empty()) return plain;
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
    MacroData data;
    data.name = fs::path(path).stem().string();
    data.botName = "GDReplay / Eclipse GDR2";

    auto bytes = readFileBytes(path);
    if (bytes.size() < 4 || bytes[0] != 'G' || bytes[1] != 'D' || bytes[2] != 'R') {
        return parseEventFile(path, "Eclipse GDR2");
    }

    ByteReader reader(bytes, 3);
    auto version = reader.varint();

    auto readVarString = [&reader]() {
        auto length = reader.varint();
        if (reader.bad || length > reader.left()) {
            reader.bad = true;
            return std::string();
        }
        return reader.str(static_cast<size_t>(length));
    };

    auto inputTag = readVarString();
    data.author = readVarString();
    readVarString(); // Description
    float duration = reader.le<float>();
    auto gameVersion = reader.varint();

    size_t fpsPos = reader.pos;
    double recordFPS = reader.le<double>();
    if (!std::isfinite(recordFPS) || recordFPS < 15.0 || recordFPS > 2000.0) {
        reader.pos = fpsPos;
        recordFPS = reader.be<double>();
    }
    if (!std::isfinite(recordFPS) || recordFPS < 15.0 || recordFPS > 2000.0) {
        reader.pos = fpsPos;
        float fpsFloat = reader.le<float>();
        if (std::isfinite(fpsFloat) && fpsFloat >= 15.0f && fpsFloat <= 2000.0f) {
            recordFPS = static_cast<double>(fpsFloat);
            reader.pos = fpsPos + sizeof(float);
        } else {
            recordFPS = 240.0;
            reader.pos = fpsPos + sizeof(double);
        }
    }
    data.recordFPS = static_cast<float>(recordFPS);

    reader.varint(); // Seed
    reader.varint(); // Coins
    reader.varint(); // Low detail mode
    bool platformer = reader.varint() != 0;
    auto botName = readVarString();
    if (!botName.empty()) data.botName = botName;
    auto botVersion = reader.varint();
    auto levelId = reader.varint();
    auto levelName = readVarString();
    if (!levelName.empty()) data.name = levelName;
    data.gameVersion = std::to_string(gameVersion);

    if (reader.bad || version == 0 || botVersion > std::numeric_limits<int>::max() ||
        levelId > std::numeric_limits<uint32_t>::max()) {
        data.error = "Invalid GDReplay header";
        return data;
    }

    auto extensionSize = reader.varint();
    if (reader.bad || extensionSize > reader.left()) {
        data.error = "Corrupted or truncated GDR2 extension data";
        return data;
    }
    reader.skip(static_cast<size_t>(extensionSize));

    auto deathCount = reader.varint();
    if (reader.bad || deathCount > reader.left()) {
        data.error = "Corrupted or truncated GDR2 death data";
        return data;
    }
    for (uint64_t death = 0; death < deathCount; death++) reader.varint();

    auto inputCount = reader.varint();
    auto playerOneInputCount = reader.varint();
    if (reader.bad || inputCount > reader.left() || playerOneInputCount > inputCount) {
        data.error = "Corrupted or truncated GDR2 input header";
        return data;
    }

    data.inputs.reserve(static_cast<size_t>(inputCount));
    int64_t previousFrame = 0;
    for (uint64_t index = 0; index < inputCount; index++) {
        auto packed = reader.varint();
        if (reader.bad) break;

        uint64_t frameDelta = platformer ? (packed >> 3) : (packed >> 1);
        if (frameDelta > static_cast<uint64_t>(std::numeric_limits<int>::max())) {
            reader.bad = true;
            break;
        }
        int64_t frame = previousFrame + static_cast<int64_t>(frameDelta);
        if (frame > std::numeric_limits<int>::max()) {
            reader.bad = true;
            break;
        }

        MacroInput input;
        input.frame = static_cast<int>(frame);
        input.subFrame = 0.0;
        input.isPress = (packed & 1) != 0;
        input.player = index < playerOneInputCount ? 1 : 2;
        input.button = platformer ? static_cast<int>((packed >> 1) & 3) : 1;
        if (input.button == 0) input.button = 1;
        input.timeSeconds = input.frame / static_cast<double>(data.recordFPS);
        input.hasTimeSeconds = true;
        data.inputs.push_back(input);

        if (!inputTag.empty()) {
            auto tagSize = reader.varint();
            if (reader.bad || tagSize > reader.left()) {
                reader.bad = true;
                break;
            }
            reader.skip(static_cast<size_t>(tagSize));
        }

        previousFrame = frame;
        if (index + 1 == playerOneInputCount) previousFrame = 0;
    }

    data.valid = !data.inputs.empty();
    if (!data.valid) {
        data.error = "GDR2 input stream is truncated or malformed";
    }
    return data;
}

MacroParser::MacroData MacroParser::parseMHRBinary(std::string const& path) {
    MacroData data;
    data.name = fs::path(path).stem().string();
    data.botName = "MegaHack Replay (Binary)";

    auto bytes = readFileBytes(path);
    static const uint8_t magic7[] = { 'H', 'A', 'C', 'K', 'P', 'R', 'O' };
    if (bytes.size() < 16 || std::memcmp(bytes.data(), magic7, 7) != 0) {
        return parseMHRJson(path);
    }

    ByteReader reader(bytes, 8);
    int32_t metadataSize = reader.le<int32_t>();
    int32_t tps = reader.le<int32_t>();
    data.recordFPS = (tps >= 30 && tps <= 2000) ? static_cast<float>(tps) : 240.0f;

    uint64_t eventHeaderOffset = 16ULL + (metadataSize > 4 ? static_cast<uint64_t>(metadataSize - 4) : 0ULL) + 8ULL;
    if (eventHeaderOffset + 8 <= bytes.size()) {
        reader.pos = static_cast<size_t>(eventHeaderOffset);
        uint32_t eventSize = reader.le<uint32_t>();
        uint32_t eventCount = reader.le<uint32_t>();
        if (!reader.bad && eventSize >= 6 && eventCount > 0 && eventCount <= (reader.left() / eventSize)) {
            data.inputs.reserve(eventCount);
            for (uint32_t index = 0; index < eventCount; index++) {
                size_t eventStart = reader.pos;
                reader.skip(2);
                bool isPress = reader.u8() == 1;
                bool isPlayerTwo = reader.u8() == 1;
                int32_t frame = reader.le<int32_t>();
                if (reader.bad || frame < 0) break;

                MacroInput input;
                input.frame = frame;
                input.timeSeconds = frame / static_cast<double>(data.recordFPS);
                input.hasTimeSeconds = true;
                input.isPress = isPress;
                input.player = isPlayerTwo ? 2 : 1;
                input.button = 1;
                data.inputs.push_back(input);
                reader.pos = eventStart + eventSize;
            }
        }
    }

    // Fallback: heuristic pattern scan for MegaHack event blocks if offset failed
    if (data.inputs.empty() && bytes.size() >= 32) {
        for (size_t offset = 16; offset + 12 <= bytes.size(); offset += 4) {
            uint32_t testSize = *reinterpret_cast<const uint32_t*>(bytes.data() + offset);
            uint32_t testCount = *reinterpret_cast<const uint32_t*>(bytes.data() + offset + 4);
            if ((testSize == 8 || testSize == 10 || testSize == 12 || testSize == 16) &&
                testCount > 0 && testCount < 500000 &&
                (offset + 8 + testSize * testCount <= bytes.size())) {
                reader.pos = offset + 8;
                for (uint32_t i = 0; i < testCount; i++) {
                    size_t eStart = reader.pos;
                    reader.skip(2);
                    bool isPress = reader.u8() == 1;
                    bool isP2 = reader.u8() == 1;
                    int32_t f = reader.le<int32_t>();
                    if (f >= 0) {
                        MacroInput inp;
                        inp.frame = f;
                        inp.timeSeconds = f / static_cast<double>(data.recordFPS);
                        inp.hasTimeSeconds = true;
                        inp.isPress = isPress;
                        inp.player = isP2 ? 2 : 1;
                        inp.button = 1;
                        data.inputs.push_back(inp);
                    }
                    reader.pos = eStart + testSize;
                }
                break;
            }
        }
    }

    data.valid = !data.inputs.empty();
    if (!data.valid) data.error = "No input events found in MegaHack binary replay";
    return data;
}

MacroParser::MacroData MacroParser::parseMHRJson(std::string const& path) {
    MacroData data;
    data.botName = "MegaHack JSON";
    data.name = fs::path(path).stem().string();

    std::string text = readFileText(path);
    if (!text.empty()) {
        try {
            auto parsed = nlohmann::json::parse(text, nullptr, false);
            if (!parsed.is_discarded()) {
                if (parsed.contains("fps") && parsed["fps"].is_number()) {
                    data.recordFPS = parsed["fps"].get<float>();
                } else if (parsed.contains("framerate") && parsed["framerate"].is_number()) {
                    data.recordFPS = parsed["framerate"].get<float>();
                }

                auto const* eventArr = parsed.contains("events") && parsed["events"].is_array()
                    ? &parsed["events"]
                    : (parsed.contains("inputs") && parsed["inputs"].is_array() ? &parsed["inputs"] : nullptr);

                if (eventArr) {
                    for (auto const& ev : *eventArr) {
                        if (!ev.is_object()) continue;
                        int frame = ev.value("frame", -1);
                        if (frame < 0) frame = ev.value("f", -1);
                        if (frame < 0) continue;

                        bool down = ev.value("down", ev.value("press", ev.value("pressed", true)));
                        bool p2 = ev.value("p2", ev.value("player2", false));
                        int btn = ev.value("button", ev.value("btn", 1));

                        MacroInput inp;
                        inp.frame = frame;
                        inp.subFrame = 0.0;
                        inp.isPress = down;
                        inp.player = p2 ? 2 : 1;
                        inp.button = btn;
                        inp.timeSeconds = frame / static_cast<double>(data.recordFPS);
                        inp.hasTimeSeconds = true;
                        data.inputs.push_back(inp);
                    }
                }
            }
        } catch (...) {}
    }

    if (!data.inputs.empty()) {
        data.valid = true;
        return data;
    }

    return parseEventFile(path, "MegaHack JSON");
}

MacroParser::MacroData MacroParser::parseXdBot(std::string const& path) {
    MacroData data;
    data.botName = "xdBot";
    data.name = fs::path(path).stem().string();
    std::ifstream input(path);
    std::string line;
    bool firstLine = true;
    while (std::getline(input, line)) {
        if (firstLine) {
            firstLine = false;
            try {
                size_t parsedLength = 0;
                float fps = std::stof(line, &parsedLength);
                if (parsedLength == line.size() && std::isfinite(fps) && fps >= 30.0f && fps <= 2000.0f) {
                    data.recordFPS = fps;
                    continue;
                }
            } catch (...) {}
        }
        if (line.empty() || line[0] == '#') continue;
        std::stringstream lineStream(line);
        std::vector<std::string> fields;
        std::string field;
        while (std::getline(lineStream, field, '|')) fields.push_back(field);
        if (fields.size() < 2) continue;

        try {
            MacroInput event;
            event.frame = std::stoi(fields[0]);
            auto state = lower(fields[1]);
            event.isPress = (state == "1" || state == "true" || state == "press" || state == "down");
            event.player = (fields.size() >= 4 && fields[3] == "0") ? 2 : 1;
            event.button = (fields.size() >= 3) ? std::max(1, std::stoi(fields[2])) : 1;
            event.timeSeconds = event.frame / static_cast<double>(data.recordFPS);
            event.hasTimeSeconds = true;
            data.inputs.push_back(event);
        } catch (...) {
            data.error = "Invalid xdBot input record";
            data.inputs.clear();
            return data;
        }
    }

    data.valid = !data.inputs.empty();
    if (!data.valid) data.error = "No xdBot input records found";
    return data;
}

MacroParser::MacroData MacroParser::parseXbot(std::string const& path) {
    MacroData data;
    data.botName = "xBot";
    data.name = fs::path(path).stem().string();
    std::ifstream input(path);
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty() || line == "frames") continue;
        if (lower(line).rfind("fps:", 0) == 0) {
            try {
                data.recordFPS = std::stof(line.substr(4));
            } catch (...) {
                data.error = "Invalid xBot FPS header";
                return data;
            }
            continue;
        }

        int state = 0;
        int frame = 0;
        std::stringstream row(line);
        if (!(row >> state >> frame)) continue;
        if (frame < 0 || data.recordFPS <= 0.0f) continue;
        MacroInput event;
        event.frame = frame;
        event.isPress = (state % 2) == 1;
        event.player = state > 1 ? 2 : 1;
        event.button = 1;
        event.timeSeconds = frame / static_cast<double>(data.recordFPS);
        event.hasTimeSeconds = true;
        data.inputs.push_back(event);
    }

    data.valid = !data.inputs.empty();
    if (!data.valid) data.error = "No xBot frame records found";
    return data;
}

MacroParser::MacroData MacroParser::parseTasBot(std::string const& path) {
    MacroData data;
    data.botName = "TASBot";
    data.name = fs::path(path).stem().string();
    auto parsed = matjson::parse(readFileText(path));
    if (!parsed) {
        data.error = "Invalid TASBot JSON";
        return data;
    }
    auto root = parsed.unwrap();
    double fps = 0.0;
    if (numberField(root, {"fps", "recordFPS", "record_fps"}, fps) && fps >= 30.0 && fps <= 2000.0) {
        data.recordFPS = static_cast<float>(fps);
    }

    auto rows = root["macro"].asArray();
    if (!rows) rows = root["inputs"].asArray();
    if (!rows) {
        data.error = "TASBot JSON has no macro/input array";
        return data;
    }
    for (auto const& row : rows.unwrap()) {
        double frame = 0.0;
        if (!numberField(row, {"frame", "frames", "tick"}, frame) || frame < 0.0) continue;
        for (int player = 1; player <= 2; player++) {
            auto key = player == 1 ? "player_1" : "player_2";
            auto playerData = row[key];
            double click = 0.0;
            if (!numberField(playerData, {"click", "state", "down", "pressed"}, click)) continue;
            MacroInput event;
            event.frame = static_cast<int>(frame);
            event.subFrame = frame - std::floor(frame);
            event.timeSeconds = frame / static_cast<double>(data.recordFPS);
            event.hasTimeSeconds = true;
            event.isPress = (click > 0.5);
            event.player = player;
            event.button = 1;
            data.inputs.push_back(event);
        }
    }

    data.valid = !data.inputs.empty();
    if (!data.valid) data.error = "No TASBot click events found";
    return data;
}

MacroParser::MacroData MacroParser::parseReplayBot(std::string const& path) {
    auto bytes = readFileBytes(path);
    if (bytes.size() < 4 || bytes[0] != 'R' || bytes[1] != 'P' || bytes[2] != 'L' || bytes[3] != 'Y') {
        return parseEventFile(path, "ReplayBot");
    }

    MacroData data;
    data.botName = "ReplayBot";
    data.name = fs::path(path).stem().string();
    if (bytes.size() < 10) {
        data.error = "ReplayBot file is truncated";
        return data;
    }

    ByteReader reader(bytes, 6);
    float recordFPS = reader.le<float>();
    data.recordFPS = (std::isfinite(recordFPS) && recordFPS >= 30.0f && recordFPS <= 2000.0f)
        ? recordFPS
        : 240.0f;
    while (reader.has(5)) {
        uint32_t frame = reader.le<uint32_t>();
        uint8_t state = reader.u8();
        if (reader.bad || frame > static_cast<uint32_t>(std::numeric_limits<int>::max())) {
            data.inputs.clear();
            data.error = "Invalid ReplayBot input event";
            return data;
        }

        MacroInput input;
        input.frame = static_cast<int>(frame);
        input.timeSeconds = frame / static_cast<double>(data.recordFPS);
        input.hasTimeSeconds = true;
        input.isPress = (state & 1) != 0;
        input.player = (state & 2) ? 2 : 1;
        input.button = 1;
        data.inputs.push_back(input);
    }

    data.valid = !data.inputs.empty();
    if (!data.valid) data.error = "No valid ReplayBot input events found";
    return data;
}

MacroParser::MacroData MacroParser::parseJsonMacro(std::string const& path) {
    return parseEventJson(path);
}

MacroParser::MacroData MacroParser::parsePlainText(std::string const& path) {
    MacroData data;
    data.name = fs::path(path).filename().string();
    std::ifstream input(path);
    std::string firstLine;
    if (std::getline(input, firstLine)) {
        auto normalized = lower(firstLine);
        if (normalized.rfind("fps:", 0) == 0 || normalized == "frames") {
            return parseXbot(path);
        }
        if (firstLine.find('|') != std::string::npos) {
            input.clear();
            input.seekg(0);
            return parseXdBot(path);
        }
        input.clear();
        input.seekg(0);
    }
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
