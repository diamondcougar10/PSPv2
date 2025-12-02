#pragma once
#include <string>
#include <optional>

struct CachedAssets {
    std::string title;
    std::string iconPath;
    std::string backgroundPath;
    std::string audioPath;
    std::string coverPath; // Usually same as background or PIC0
};

class RomAssetManager {
public:
    // Returns paths to cached assets, extracting them if necessary
    static CachedAssets getOrExtractAssets(const std::string& romPath, const std::string& cacheRoot);
};
