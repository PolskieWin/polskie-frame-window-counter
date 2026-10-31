#include "MacroParser.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <Geode/utils/file.hpp>

namespace fs = std::filesystem;

MacroParser::MacroFormat MacroParser::detectFormat(std::string const& path) {
    std::string ext = fs::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    std::string filename = fs::path(path).filename().string();
    std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);

    if (ext == ".gdr2" || ext == ".gdr") {
        return MacroFormat::ECLIPSE_GDR2;
    }
    if (filename.ends_with(".mhr.json") || ext == ".mhr") {
        return MacroFormat::MHR_JSON;
    }
    if (ext == ".json") {
        return MacroFormat::JSON;
    }

    return MacroFormat::UNKNOWN;
}

MacroParser::MacroData MacroParser::parse(std::string const& path) {
    auto format = detectFormat(path);

    switch (format) {
        case MacroFormat::ECLIPSE_GDR2:
            return parseEclipseGDR2(path);
        case MacroFormat::MHR_JSON:
            return parseMHRJson(path);
        case MacroFormat::JSON:
            return parseGenericJson(path);
        default: {
            MacroData data;
            data.valid = false;
            data.error = "Nieznany format pliku: " + fs::path(path).extension().string();
            return data;
        }
    }
}

// Helper: read unsigned varint
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
            data.error = "Nie można otworzyć pliku makra Eclipse: " + path;
            return data;
        }

        // GDR2 Header:
        // Format typically has:
        // 4 bytes: Magic ("GDR2" or version byte)
        // FPS float or varint
        // level name string / length
        char magic[4] = {};
        file.read(magic, 4);

        float fps = 240.0f;
        file.read(reinterpret_cast<char*>(&fps), sizeof(float));
        if (fps >= 30.0f && fps <= 1000.0f) {
            data.recordFPS = fps;
        } else {
            data.recordFPS = 240.0f;
        }

        // Read total inputs count
        uint64_t inputCount = readVarint(file);
        if (inputCount == 0 || inputCount > 500000) {
            // Might be alternative header without float FPS, try reading varint directly
            file.seekg(4, std::ios::beg);
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
            action.button = (flags >> 2) & 0x03; // 0=jump, 1=left, 2=right
            if (action.button == 0) action.button = 1;

            data.inputs.push_back(action);
        }

        if (data.inputs.empty()) {
            data.error = "Brak klatek w pliku makra Eclipse (.gdr2)";
            data.valid = false;
        } else {
            data.valid = true;
            log::info("[Polskie FWC] Załadowano makro Eclipse (GDR2): {} kliknięć przy {:.0f}fps",
                data.inputs.size(), data.recordFPS);
        }

    } catch (std::exception const& e) {
        data.error = std::string("Błąd parsowania Eclipse GDR2: ") + e.what();
    }

    return data;
}

// ============================================================
// MegaHack Replay (.mhr / .mhr.json) parser
// ============================================================
MacroParser::MacroData MacroParser::parseMHRJson(std::string const& path) {
    MacroData data;
    data.botName = "MegaHack Replay";
    data.name = fs::path(path).stem().string();

    try {
        auto result = file::readString(fs::path(path));
        if (!result) {
            data.error = "Nie udało się odczytać pliku: " + path;
            return data;
        }

        std::string content = result.unwrap();
        auto jsonRes = matjson::parse(content);
        if (!jsonRes) {
            data.error = "Błąd JSON MegaHack: " + jsonRes.unwrapErr().message;
            return data;
        }
        auto json = jsonRes.unwrap();

        if (json.contains("name")) data.name = json["name"].asString().unwrapOr(data.name);
        if (json.contains("author")) data.author = json["author"].asString().unwrapOr("");
        if (json.contains("fps")) data.recordFPS = static_cast<float>(json["fps"].asDouble().unwrapOr(240.0));
        if (json.contains("version")) data.botName += " " + json["version"].asString().unwrapOr("");

        if (json.contains("inputs") && json["inputs"].isArray()) {
            auto& inputs = json["inputs"];
            for (size_t i = 0; i < inputs.size(); i++) {
                auto& inp = inputs[i];
                MacroInput action;
                action.frame = static_cast<int>(inp["frame"].asInt().unwrapOr(0));
                action.isPress = inp["down"].asBool().unwrapOr(true);
                action.player = static_cast<int>(inp["player"].asInt().unwrapOr(1));
                action.button = 1;
                data.inputs.push_back(action);
            }
        }

        std::sort(data.inputs.begin(), data.inputs.end(),
            [](MacroInput const& a, MacroInput const& b) {
                return a.frame < b.frame;
            }
        );

        data.valid = !data.inputs.empty();
        if (!data.valid) {
            data.error = "Brak danych wejściowych w pliku MegaHack Replay";
        }

    } catch (std::exception const& e) {
        data.error = std::string("Błąd JSON MegaHack: ") + e.what();
    }

    return data;
}

// ============================================================
// Generic JSON / TASBot / xdBot parser
// ============================================================
MacroParser::MacroData MacroParser::parseGenericJson(std::string const& path) {
    MacroData data;
    data.botName = "Generic Bot JSON";
    data.name = fs::path(path).stem().string();

    try {
        auto result = file::readString(fs::path(path));
        if (!result) {
            data.error = "Nie udało się otworzyć pliku: " + path;
            return data;
        }

        std::string content = result.unwrap();
        auto jsonRes = matjson::parse(content);
        if (!jsonRes) {
            data.error = "Błąd parsowania JSON: " + jsonRes.unwrapErr().message;
            return data;
        }
        auto json = jsonRes.unwrap();

        if (json.contains("fps")) {
            data.recordFPS = static_cast<float>(json["fps"].asDouble().unwrapOr(240.0));
        } else if (json.contains("framerate")) {
            data.recordFPS = static_cast<float>(json["framerate"].asDouble().unwrapOr(240.0));
        }

        std::string inputKey = "";
        if (json.contains("actions")) inputKey = "actions";
        else if (json.contains("inputs")) inputKey = "inputs";
        else if (json.contains("macro")) inputKey = "macro";
        else if (json.contains("events")) inputKey = "events";

        if (!inputKey.empty() && json[inputKey].isArray()) {
            auto& inputs = json[inputKey];
            for (size_t i = 0; i < inputs.size(); i++) {
                auto& inp = inputs[i];
                MacroInput action;

                if (inp.contains("frame")) {
                    action.frame = static_cast<int>(inp["frame"].asInt().unwrapOr(0));
                } else if (inp.contains("f")) {
                    action.frame = static_cast<int>(inp["f"].asInt().unwrapOr(0));
                } else if (inp.contains("time")) {
                    action.frame = static_cast<int>(inp["time"].asDouble().unwrapOr(0.0) * data.recordFPS);
                }

                if (inp.contains("hold")) {
                    action.isPress = inp["hold"].asBool().unwrapOr(true);
                } else if (inp.contains("down")) {
                    action.isPress = inp["down"].asBool().unwrapOr(true);
                } else if (inp.contains("press")) {
                    action.isPress = inp["press"].asBool().unwrapOr(true);
                }

                action.player = inp.contains("player") ? static_cast<int>(inp["player"].asInt().unwrapOr(1)) : 1;
                action.button = 1;
                data.inputs.push_back(action);
            }
        }

        std::sort(data.inputs.begin(), data.inputs.end(),
            [](MacroInput const& a, MacroInput const& b) {
                return a.frame < b.frame;
            }
        );

        data.valid = !data.inputs.empty();

    } catch (std::exception const& e) {
        data.error = std::string("Błąd parsowania JSON: ") + e.what();
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

            if (ext == ".gdr2" || ext == ".gdr" || ext == ".mhr" || ext == ".json" ||
                name.ends_with(".mhr.json")) {
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

    // Eclipse Menu specific folders
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
