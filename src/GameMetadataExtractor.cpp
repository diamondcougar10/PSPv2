#include "GameMetadataExtractor.hpp"
#include <fstream>
#include <iostream>
#include <cstring>
#include <algorithm>
#include <map>

// Helper to read Little Endian integers
static uint32_t readLE32(std::ifstream& file) {
    uint32_t val;
    file.read(reinterpret_cast<char*>(&val), 4);
    return val;
}

static uint16_t readLE16(std::ifstream& file) {
    uint16_t val;
    file.read(reinterpret_cast<char*>(&val), 2);
    return val;
}

// PBP Header
struct PbpHeader {
    char magic[4];
    uint32_t version;
    uint32_t offsetParamSfo;
    uint32_t offsetIcon0;
    uint32_t offsetIcon1;
    uint32_t offsetPic0;
    uint32_t offsetPic1;
    uint32_t offsetSnd0;
    uint32_t offsetDataPsp;
    uint32_t offsetDataPsar;
};

// SFO Parser
static std::string parseSfoString(const std::vector<uint8_t>& sfoData, const std::string& key) {
    if (sfoData.size() < 20) return "";

    const uint8_t* data = sfoData.data();
    // Magic check "\0PSF"
    if (memcmp(data, "\0PSF", 4) != 0) return "";

    uint32_t keyTableOffset = *reinterpret_cast<const uint32_t*>(data + 0x08);
    uint32_t dataTableOffset = *reinterpret_cast<const uint32_t*>(data + 0x0C);
    uint32_t entries = *reinterpret_cast<const uint32_t*>(data + 0x10);

    for (uint32_t i = 0; i < entries; ++i) {
        uint32_t entryOffset = 0x14 + (i * 16);
        if (entryOffset + 16 > sfoData.size()) break;

        uint16_t keyOffset = *reinterpret_cast<const uint16_t*>(data + entryOffset);
        uint32_t dataLen = *reinterpret_cast<const uint32_t*>(data + entryOffset + 4);
        uint32_t dataOffset = *reinterpret_cast<const uint32_t*>(data + entryOffset + 12);

        if (keyTableOffset + keyOffset >= sfoData.size()) continue;
        std::string currentKey(reinterpret_cast<const char*>(data + keyTableOffset + keyOffset));

        if (currentKey == key) {
            if (dataTableOffset + dataOffset + dataLen > sfoData.size()) return "";
            return std::string(reinterpret_cast<const char*>(data + dataTableOffset + dataOffset), dataLen);
        }
    }
    return "";
}

GameMetadata GameMetadataExtractor::extract(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) return {};

    char magic[4];
    file.read(magic, 4);
    file.close();

    if (memcmp(magic, "\0PBP", 4) == 0) {
        return extractFromPbp(filePath);
    } else {
        // Assume ISO if not PBP
        return extractFromIso(filePath);
    }
}

GameMetadata GameMetadataExtractor::extractFromPbp(const std::string& filePath) {
    GameMetadata meta;
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) return meta;

    PbpHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));

    // Helper to read a section
    auto readSection = [&](uint32_t start, uint32_t end) -> std::vector<uint8_t> {
        if (end <= start) return {};
        std::vector<uint8_t> buffer(end - start);
        file.seekg(start);
        file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
        return buffer;
    };

    // PARAM.SFO
    auto sfoData = readSection(header.offsetParamSfo, header.offsetIcon0);
    meta.title = parseSfoString(sfoData, "TITLE");
    meta.gameId = parseSfoString(sfoData, "DISC_ID");

    // ICON0.PNG
    meta.iconData = readSection(header.offsetIcon0, header.offsetIcon1);

    // PIC1.PNG
    meta.backgroundData = readSection(header.offsetPic1, header.offsetSnd0);

    // SND0.AT3
    meta.soundData = readSection(header.offsetSnd0, header.offsetDataPsp);

    return meta;
}

// ISO 9660 Minimal Parser
struct IsoDirEntry {
    uint32_t lba;
    uint32_t size;
    bool isDirectory;
    std::string name;
};

static std::vector<IsoDirEntry> readIsoDirectory(std::ifstream& file, uint32_t lba, uint32_t size) {
    std::vector<IsoDirEntry> entries;
    file.seekg(lba * 2048);
    std::vector<uint8_t> buffer(size);
    file.read(reinterpret_cast<char*>(buffer.data()), size);

    size_t offset = 0;
    while (offset < size) {
        uint8_t len = buffer[offset];
        if (len == 0) {
            // Padding or end of sector, align to next sector boundary if needed, but usually just skip
            // In ISO 9660, directory records don't cross sector boundaries.
            // If we hit 0, we scan forward to next 2048 boundary?
            // Actually, if len is 0, it's usually padding until the end of the sector.
            // We should calculate remaining bytes in this sector.
            size_t currentSectorOffset = offset % 2048;
            size_t remaining = 2048 - currentSectorOffset;
            offset += remaining;
            continue;
        }
        
        if (offset + 33 > size) break;

        uint32_t extent = *reinterpret_cast<uint32_t*>(&buffer[offset + 2]); // LE
        uint32_t dataLen = *reinterpret_cast<uint32_t*>(&buffer[offset + 10]); // LE
        uint8_t flags = buffer[offset + 25];
        uint8_t nameLen = buffer[offset + 32];
        
        std::string name;
        if (nameLen == 1 && buffer[offset + 33] == 0) name = ".";
        else if (nameLen == 1 && buffer[offset + 33] == 1) name = "..";
        else name = std::string(reinterpret_cast<char*>(&buffer[offset + 33]), nameLen);

        // Remove version ;1
        size_t semi = name.find(';');
        if (semi != std::string::npos) {
            name = name.substr(0, semi);
        }

        entries.push_back({extent, dataLen, (flags & 2) != 0, name});
        offset += len;
    }
    return entries;
}

GameMetadata GameMetadataExtractor::extractFromIso(const std::string& filePath) {
    GameMetadata meta;
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) return meta;

    // Read PVD at sector 16
    file.seekg(16 * 2048);
    uint8_t pvd[2048];
    file.read(reinterpret_cast<char*>(pvd), 2048);

    if (memcmp(pvd + 1, "CD001", 5) != 0) return meta; // Not a valid ISO

    // Root Directory Record starts at 156
    uint32_t rootLba = *reinterpret_cast<uint32_t*>(pvd + 156 + 2);
    uint32_t rootSize = *reinterpret_cast<uint32_t*>(pvd + 156 + 10);

    auto rootEntries = readIsoDirectory(file, rootLba, rootSize);

    // Find PSP_GAME directory
    uint32_t pspGameLba = 0;
    uint32_t pspGameSize = 0;
    for (const auto& entry : rootEntries) {
        if (entry.isDirectory && entry.name == "PSP_GAME") {
            pspGameLba = entry.lba;
            pspGameSize = entry.size;
            break;
        }
    }

    if (pspGameLba == 0) return meta;

    auto gameEntries = readIsoDirectory(file, pspGameLba, pspGameSize);

    for (const auto& entry : gameEntries) {
        if (!entry.isDirectory) {
            if (entry.name == "PARAM.SFO") {
                file.seekg(entry.lba * 2048);
                std::vector<uint8_t> sfoData(entry.size);
                file.read(reinterpret_cast<char*>(sfoData.data()), entry.size);
                meta.title = parseSfoString(sfoData, "TITLE");
                meta.gameId = parseSfoString(sfoData, "DISC_ID");
            } else if (entry.name == "ICON0.PNG") {
                file.seekg(entry.lba * 2048);
                meta.iconData.resize(entry.size);
                file.read(reinterpret_cast<char*>(meta.iconData.data()), entry.size);
            } else if (entry.name == "PIC1.PNG") {
                file.seekg(entry.lba * 2048);
                meta.backgroundData.resize(entry.size);
                file.read(reinterpret_cast<char*>(meta.backgroundData.data()), entry.size);
            } else if (entry.name == "SND0.AT3") {
                file.seekg(entry.lba * 2048);
                meta.soundData.resize(entry.size);
                file.read(reinterpret_cast<char*>(meta.soundData.data()), entry.size);
            }
        }
    }

    return meta;
}
