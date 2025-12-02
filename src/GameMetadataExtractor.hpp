#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <optional>

struct GameMetadata {
    std::string title;
    std::string gameId;
    std::vector<uint8_t> iconData;       // ICON0.PNG
    std::vector<uint8_t> backgroundData; // PIC1.PNG
    std::vector<uint8_t> soundData;      // SND0.AT3 (Raw data, might not be playable directly)
};

class GameMetadataExtractor {
public:
    static GameMetadata extract(const std::string& filePath);

private:
    static GameMetadata extractFromIso(const std::string& filePath);
    static GameMetadata extractFromPbp(const std::string& filePath);
};
