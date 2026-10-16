#pragma once
#include <vector>
#include <string>
#include <fstream>
#include "types.h"

namespace MacroFile {

const uint32_t MAGIC_HEADER = 0x4345524B; // 'KREC' in little endian
const uint16_t FORMAT_VERSION = 1;

#pragma pack(push, 1)
struct FileHeader {
    uint32_t magic;      // 'KREC'
    uint16_t version;    // 1
    uint32_t count;      // Number of events
};
#pragma pack(pop)

inline bool SaveToFile(const std::wstring& filePath, const std::vector<KeyEvent>& events) {
    std::ofstream out(filePath.c_str(), std::ios::binary);
    if (!out.is_open()) return false;

    FileHeader header;
    header.magic = MAGIC_HEADER;
    header.version = FORMAT_VERSION;
    header.count = static_cast<uint32_t>(events.size());

    out.write(reinterpret_cast<const char*>(&header), sizeof(header));
    if (!events.empty()) {
        out.write(reinterpret_cast<const char*>(events.data()), events.size() * sizeof(KeyEvent));
    }
    return out.good();
}

inline bool LoadFromFile(const std::wstring& filePath, std::vector<KeyEvent>& outEvents) {
    std::ifstream in(filePath.c_str(), std::ios::binary);
    if (!in.is_open()) return false;

    FileHeader header;
    in.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (!in.good() || header.magic != MAGIC_HEADER || header.version != FORMAT_VERSION) {
        return false;
    }

    outEvents.resize(header.count);
    if (header.count > 0) {
        in.read(reinterpret_cast<char*>(outEvents.data()), header.count * sizeof(KeyEvent));
        if (!in.good()) {
            outEvents.clear();
            return false;
        }
    }
    return true;
}

} // namespace MacroFile
