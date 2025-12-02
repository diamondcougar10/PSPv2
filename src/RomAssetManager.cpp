#include "RomAssetManager.hpp"
#include "GameMetadataExtractor.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>

namespace fs = std::filesystem;

static std::string sanitizeFilename(std::string name) {
    std::replace(name.begin(), name.end(), ':', '_');
    std::replace(name.begin(), name.end(), '/', '_');
    std::replace(name.begin(), name.end(), '\\', '_');
    std::replace(name.begin(), name.end(), '<', '_');
    std::replace(name.begin(), name.end(), '>', '_');
    std::replace(name.begin(), name.end(), '*', '_');
    std::replace(name.begin(), name.end(), '?', '_');
    std::replace(name.begin(), name.end(), '|', '_');
    std::replace(name.begin(), name.end(), '"', '_');
    return name;
}

static void writeDataToFile(const std::string& path, const std::vector<uint8_t>& data) {
    if (data.empty()) return;
    if (fs::exists(path)) return; // Don't overwrite

    std::ofstream file(path, std::ios::binary);
    if (file) {
        file.write(reinterpret_cast<const char*>(data.data()), data.size());
        std::cout << "Cached asset: " << path << "\n";
    }
}

CachedAssets RomAssetManager::getOrExtractAssets(const std::string& romPath, const std::string& cacheRoot) {
    CachedAssets assets;
    
    // 1. Determine Game ID from filename first (to allow cache check)
    std::string filename = fs::path(romPath).stem().string();
    std::string gameId = sanitizeFilename(filename);
    std::string gameCacheDir = cacheRoot + "/" + gameId;

    // 2. Check if assets already exist
    std::string iconPath = gameCacheDir + "/ICON0.PNG";
    std::string bgPath = gameCacheDir + "/PIC1.PNG";
    std::string sndPath = gameCacheDir + "/SND0.AT3";
    std::string wavPath = gameCacheDir + "/PREVIEW.WAV";
    
    bool iconExists = fs::exists(iconPath);
    bool bgExists = fs::exists(bgPath);
    
    if (iconExists || bgExists) {
        std::cout << "[RomAssetManager] Cache hit for " << gameId << "\n";
        if (iconExists) assets.iconPath = iconPath;
        if (bgExists) {
            assets.backgroundPath = bgPath;
            assets.coverPath = bgPath;
        }
        if (fs::exists(wavPath)) {
            assets.audioPath = wavPath;
        } else if (fs::exists(sndPath)) {
             // Try to convert if we have AT3 but no WAV (e.g. previous run failed)
            std::string cmd = "ffmpeg -y -i \"" + sndPath + "\" -acodec pcm_s16le -ar 44100 \"" + wavPath + "\" > nul 2>&1";
            if (std::system(cmd.c_str()) == 0 && fs::exists(wavPath)) {
                assets.audioPath = wavPath;
            } else {
                assets.audioPath = sndPath;
            }
        }
        
        // Try to read title from a sidecar file if we had one, otherwise leave empty
        // (Menu will use filename)
        return assets;
    }

    std::cout << "[RomAssetManager] Cache miss for " << gameId << ", extracting...\n";

    // 3. Extract metadata from ROM
    GameMetadata meta;
    try {
        meta = GameMetadataExtractor::extract(romPath);
    } catch (const std::exception& e) {
        std::cerr << "Error extracting metadata from " << romPath << ": " << e.what() << "\n";
        return assets;
    }

    assets.title = meta.title;

    // If we got a real ID from the ISO, we could use it, but to keep consistent with the cache check above,
    // we should stick to the filename-based ID. 
    // Or we could migrate? No, let's stick to filename for simplicity as requested.
    
    try {
        fs::create_directories(gameCacheDir);
    } catch (const std::exception& e) {
        std::cerr << "Failed to create cache dir " << gameCacheDir << ": " << e.what() << "\n";
        return assets;
    }

    // 4. Save assets and populate paths
    if (!meta.iconData.empty()) {
        writeDataToFile(iconPath, meta.iconData);
        assets.iconPath = iconPath;
    }

    if (!meta.backgroundData.empty()) {
        writeDataToFile(bgPath, meta.backgroundData);
        assets.backgroundPath = bgPath;
        assets.coverPath = bgPath; // Use PIC1 as cover for now
    }

    if (!meta.soundData.empty()) {
        writeDataToFile(sndPath, meta.soundData);
        
        // Convert AT3 to WAV using ffmpeg if possible
        std::string wavPath = gameCacheDir + "/PREVIEW.WAV";
        if (!fs::exists(wavPath)) {
            std::string cmd = "ffmpeg -y -i \"" + sndPath + "\" -acodec pcm_s16le -ar 44100 \"" + wavPath + "\" > nul 2>&1";
            int result = std::system(cmd.c_str());
            if (result == 0 && fs::exists(wavPath)) {
                assets.audioPath = wavPath;
                std::cout << "Converted audio to WAV: " << wavPath << "\n";
            } else {
                // Fallback to AT3 path (won't play in SFML but keeps the reference)
                assets.audioPath = sndPath; 
                std::cerr << "Failed to convert AT3 to WAV (ffmpeg missing?)\n";
            }
        } else {
            assets.audioPath = wavPath;
        }
    }

    return assets;
}
