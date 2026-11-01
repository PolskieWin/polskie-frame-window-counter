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
        auto res = file::readString(path);
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
