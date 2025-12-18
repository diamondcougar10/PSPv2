#include "Menu.hpp"
#include "UserProfile.hpp"
#include "UiSoundBank.hpp"
#include "RomAssetManager.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <ctime>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <windows.h>
#include <filesystem>
#include <cstdint>
#include <shlobj.h> // For SHGetKnownFolderPath

using json = nlohmann::json;

// Logger helper for debugging
class ScanLogger {
public:
    std::ofstream logFile;
    
    ScanLogger() {
        // Get AppData path for logs
        PWSTR path = NULL;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &path))) {
            std::filesystem::path logsDir = std::filesystem::path(path) / "PSPV2" / "logs";
            CoTaskMemFree(path);
            std::filesystem::create_directories(logsDir);
            
            // Create log file with timestamp
            auto now = std::time(nullptr);
            auto tm = std::localtime(&now);
            std::ostringstream filename;
            filename << "scan_" << std::put_time(tm, "%Y%m%d_%H%M%S") << ".log";
            
            logFile.open(logsDir / filename.str());
            if (logFile) {
                logFile << "=== PSPV2 ROM Scan Log ===" << std::endl;
                logFile << "Time: " << std::put_time(tm, "%Y-%m-%d %H:%M:%S") << std::endl;
                logFile << std::endl;
            }
        }
    }
    
    void log(const std::string& msg) {
        if (logFile) {
            logFile << msg << std::endl;
            logFile.flush();
        }
        std::cout << msg << std::endl;
    }
    
    ~ScanLogger() {
        if (logFile) {
            logFile << "\n=== End of Log ===" << std::endl;
            logFile.close();
        }
    }
};

Menu::Menu(const std::string& configPath, UiSoundBank& sounds, UserProfile* profile)
    : soundBank_(sounds), userProfile_(profile) {
  // Load font
  if (!font_.openFromFile("assets/fonts/ui_font.ttf")) {
    fontLoaded_ = false;
    std::cerr << "Warning: failed to load font at assets/fonts/ui_font.ttf\n";
  } else {
    fontLoaded_ = true;
  }

  // Load background from profile theme or default
  reloadBackground();

  // Load settings to get games_root path
  loadSettings("config/settings.json");
  
  // Load menu structure and assets
  loadFromFile(configPath);
  scanRomsFolder();  // Auto-scan Downloads folder for ROMs
  loadAssets();
}

void Menu::reloadBackground() {
  std::string bgPath = "assets/Themes/Background.png";
  
  // Use theme from profile if available
  if (userProfile_ && !userProfile_->getTheme().empty() && userProfile_->getTheme() != "default") {
    bgPath = "assets/Backgrounds/" + userProfile_->getTheme();
  }
  
  try {
    bgTexture_ = sf::Texture(bgPath);
    bgLoaded_ = true;
    bgSprite_ = sf::Sprite(bgTexture_);
    bgSprite_->setTextureRect(sf::IntRect({0, 0}, sf::Vector2i(bgTexture_.getSize())));
    // Scale background slightly larger for parallax effect
    float scaleX = 1320.f / bgTexture_.getSize().x;  // 40px extra for movement
    float scaleY = 760.f / bgTexture_.getSize().y;   // 40px extra for movement
    bgSprite_->setScale({scaleX, scaleY});
    bgSprite_->setPosition({-20.f, -20.f}); // Center the extra space
    std::cout << "Loaded background: " << bgPath << "\n";
  } catch (const std::exception& e) {
    std::cerr << "Warning: failed to load " << bgPath << ": " << e.what() << "\n";
    bgLoaded_ = false;
  }
}

void Menu::loadFromFile(const std::string& configPath) {
  std::ifstream ifs(configPath);
  if (!ifs) {
    std::cerr << "Failed to open menu config: " << configPath << "\n";
    return;
  }

  try {
    json j;
    ifs >> j;
    if (!j.contains("categories") || !j["categories"].is_array()) return;

    for (const auto& catJson : j["categories"]) {
      Category cat;
      cat.id = catJson.value("id", std::string("unknown"));
      cat.label = catJson.value("label", std::string("Unknown"));
      cat.iconFilename = catJson.value("icon", std::string(""));

      if (catJson.contains("items") && catJson["items"].is_array()) {
        for (const auto& itemJson : catJson["items"]) {
          MenuItem item;
          item.label = itemJson.value("label", std::string("Unnamed"));
          item.path = itemJson.value("path", std::string(""));
          item.type = itemJson.value("type", std::string("pc_app"));
          item.iconFilename = itemJson.value("icon", std::string(""));
          
          // Preview fields
          item.previewImagePath = itemJson.value("preview_image", std::string(""));
          if (item.previewImagePath.empty()) {
              item.previewImagePath = itemJson.value("preview_card", std::string(""));
          }
          item.coverArtPath = itemJson.value("cover_art", std::string(""));
          item.previewBgPath = itemJson.value("preview_bg", std::string(""));
          item.previewAudioPath = itemJson.value("preview_audio", std::string(""));

          cat.items.push_back(item);
        }
      }

      categories_.push_back(cat);
    }
  } catch (const std::exception& e) {
    std::cerr << "Error parsing menu config: " << e.what() << "\n";
  }
}

void Menu::loadAssets() {
  // Load icon textures for each category from Icons folder
  for (auto& cat : categories_) {
    if (cat.iconFilename.empty()) {
      std::cerr << "Warning: No icon specified for category " << cat.id << "\n";
      continue;
    }
    
    std::string iconPath = "assets/Icons/" + cat.iconFilename;
    try {
      cat.iconTex = sf::Texture(iconPath);
      cat.iconTex.setSmooth(true); // Enable smooth filtering for zoomed icons
      cat.iconSprite = sf::Sprite(cat.iconTex);
      // Center origin for scaling
      sf::Vector2u size = cat.iconTex.getSize();
      cat.iconSprite->setOrigin({size.x / 2.f, size.y / 2.f});
      std::cout << "Loaded icon: " << iconPath << " for category " << cat.label << "\n";
    } catch (const std::exception& e) {
      std::cerr << "Warning: failed to load icon " << iconPath << ": " << e.what() << "\n";
    }
    
    // Load icon textures for each item
    for (auto& item : cat.items) {
      if (!item.iconFilename.empty()) {
        std::string itemIconPath = "assets/Icons/" + item.iconFilename;
        try {
          item.iconTex = sf::Texture(itemIconPath);
          item.iconTex.setSmooth(true); // Enable smooth filtering
          item.iconSprite = sf::Sprite(item.iconTex);
          std::cout << "Loaded icon: " << itemIconPath << " for item " << item.label << "\n";
        } catch (const std::exception& e) {
          std::cerr << "Warning: failed to load item icon " << itemIconPath << ": " << e.what() << "\n";
        }
      }

      // Load preview image
      if (!item.previewImagePath.empty()) {
        if (item.previewTexture.loadFromFile(item.previewImagePath)) {
            item.previewSprite.emplace(item.previewTexture);
            item.hasPreview = true;
            std::cout << "Loaded preview: " << item.previewImagePath << "\n";
        } else {
            std::cerr << "Warning: failed to load preview " << item.previewImagePath << "\n";
        }
      }

      // Load preview background
      if (!item.previewBgPath.empty()) {
        if (item.previewBgTexture.loadFromFile(item.previewBgPath)) {
            item.previewBgTexture.setSmooth(true);
            item.previewBgSprite.emplace(item.previewBgTexture);
            item.hasPreviewBg = true;
            std::cout << "Loaded preview bg: " << item.previewBgPath << "\n";
        } else {
            std::cerr << "Warning: failed to load preview bg " << item.previewBgPath << "\n";
        }
      }

      // Load cover art
      if (!item.coverArtPath.empty()) {
        if (item.coverArtTexture.loadFromFile(item.coverArtPath)) {
            item.coverArtTexture.setSmooth(true);
            item.coverArtSprite.emplace(item.coverArtTexture);
            item.hasCoverArt = true;
            std::cout << "Loaded cover art: " << item.coverArtPath << "\n";
        } else {
            std::cerr << "Warning: failed to load cover art " << item.coverArtPath << "\n";
        }
      }

      // Load preview audio
      if (!item.previewAudioPath.empty()) {
        item.previewBuffer = std::make_shared<sf::SoundBuffer>();
        if (item.previewBuffer->loadFromFile(item.previewAudioPath)) {
            item.hasPreviewAudio = true;
            std::cout << "Loaded preview audio: " << item.previewAudioPath << "\n";
        } else {
            std::cerr << "Warning: failed to load preview audio " << item.previewAudioPath << "\n";
            item.previewBuffer.reset();
        }
      }
    }
  }
}

void Menu::loadSettings(const std::string& settingsPath) {
  std::ifstream ifs(settingsPath);
  if (!ifs) {
    std::cerr << "Warning: Failed to open settings file: " << settingsPath << "\n";
    gamesRoot_ = "Games";  // Fallback to relative path
    return;
  }

  try {
    json j;
    ifs >> j;
    gamesRoot_ = j.value("games_root", std::string("Games"));
    std::cout << "Menu: Using games root: " << gamesRoot_ << "\n";
  } catch (const std::exception& e) {
    std::cerr << "Error parsing settings.json: " << e.what() << "\n";
    gamesRoot_ = "Games";  // Fallback
  }
}

void Menu::scanRomsFolder() {
  // Create logger for this scan
  ScanLogger logger;
  
  // Helper lambda to check file extension (C++17 compatible)
  auto hasExtension = [](const std::string& filename, const std::string& ext) {
    if (filename.length() < ext.length()) return false;
    return filename.compare(filename.length() - ext.length(), ext.length(), ext) == 0;
  };
  
  // Get the executable's directory to ensure we're looking in the right place
  wchar_t exePath[MAX_PATH];
  GetModuleFileNameW(NULL, exePath, MAX_PATH);
  std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
  
  logger.log("Executable directory: " + exeDir.string());
  
  // Resolve Games path relative to executable location
  std::filesystem::path gamesPath = exeDir / gamesRoot_;
  
  // If it doesn't exist, try current working directory as fallback
  if (!std::filesystem::exists(gamesPath)) {
      logger.log("Games folder not found at exe location, trying fallbacks...");
      gamesPath = std::filesystem::absolute(gamesRoot_);
      if (!std::filesystem::exists(gamesPath)) {
          // Try going up levels from current directory
          std::vector<std::string> tryPaths = {
              "../" + gamesRoot_,
              "../../" + gamesRoot_,
              "../../../" + gamesRoot_
          };
          
          for (const auto& p : tryPaths) {
              if (std::filesystem::exists(p)) {
                  gamesPath = std::filesystem::absolute(p);
                  logger.log("Found Games folder at: " + gamesPath.string());
                  break;
              }
          }
      }
  }
  
  logger.log("Using Games folder: " + gamesPath.string());
  
  // FIX 1: Ensure Games folder exists (create if it doesn't)
  std::error_code ec;
  std::filesystem::create_directories(gamesPath, ec);
  if (ec) {
      logger.log("ERROR: Failed to create Games folder: " + gamesPath.string() + " | " + ec.message());
      // Don't return - try to continue anyway
  }
  
  // Use the user's actual Downloads folder
  PWSTR downloadsPathStr = NULL;
  std::filesystem::path downloadPath;
  if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Downloads, 0, NULL, &downloadsPathStr))) {
      downloadPath = std::filesystem::path(downloadsPathStr);
      CoTaskMemFree(downloadsPathStr);
      logger.log("Scanning User Downloads: " + downloadPath.string());
  } else {
      downloadPath = std::filesystem::absolute("Downloads"); // Fallback
      logger.log("WARNING: Could not get user Downloads, using fallback: " + downloadPath.string());
  }
  
  // Verify both paths exist
  logger.log("Games folder exists: " + std::string(std::filesystem::exists(gamesPath) ? "YES" : "NO"));
  logger.log("Downloads folder exists: " + std::string(std::filesystem::exists(downloadPath) ? "YES" : "NO"));

  // Find the Games category index first
  size_t gamesIdx = 0;
  bool gamesCategoryFound = false;
  for (size_t i = 0; i < categories_.size(); ++i) {
    if (categories_[i].id == "games") {
      gamesIdx = i;
      gamesCategoryFound = true;
      break;
    }
  }

  if (!gamesCategoryFound) {
      logger.log("ERROR: 'games' category not found in menu configuration.");
      return;
  }

  int romsFound = 0;
  int archivesProcessed = 0;
  WIN32_FIND_DATAA findData;
  
  // Scan Downloads folder for PSP ROMs
  std::string searchPath = downloadPath.string() + "/*.*";
  logger.log("Search path: " + searchPath);
  
  HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
  
  if (hFind == INVALID_HANDLE_VALUE) {
      logger.log("ERROR: FindFirstFileA failed or no files. Error code: " + std::to_string(GetLastError()));
  }
  
  if (hFind != INVALID_HANDLE_VALUE) {
      logger.log("Scanning Downloads folder...");
      // First pass: Move ROM files from Downloads to Games folder
      do {
        std::string filename = findData.cFileName;
        if (filename == "." || filename == "..") continue;
        
        std::string lower = filename;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        
        bool isRom = hasExtension(lower, ".iso") || hasExtension(lower, ".cso") || hasExtension(lower, ".pbp");
        
        // Also check for ZIP files to extract
        bool isZip = hasExtension(lower, ".zip") || hasExtension(lower, ".7z") || hasExtension(lower, ".rar");
        
        if (isZip) {
             // Extract any zip/7z/rar file - user explicitly put it in Downloads
             // so we assume they want it extracted to Games
             logger.log("Found archive: " + filename);
             std::filesystem::path sourcePath = downloadPath / filename;
             
             // Get log directory for capturing extraction output
             std::filesystem::path extractLogPath;
             {
                 PWSTR appDataPath = NULL;
                 if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &appDataPath))) {
                     extractLogPath = std::filesystem::path(appDataPath) / "PSPV2" / "logs" / ("extract_" + filename + ".log");
                     CoTaskMemFree(appDataPath);
                 }
             }
             
             std::string cmd;
             if (hasExtension(lower, ".zip")) {
                 // Use PowerShell for .zip files (built-in on Windows)
                 // We use single quotes for paths to handle spaces
                 cmd = "powershell -command \"Expand-Archive -Path '" + sourcePath.string() + "' -DestinationPath '" + gamesPath.string() + "' -Force\"";
             } else {
                 // Try bundled 7z first, then system 7z
                 std::filesystem::path bundled7z = exeDir / "tools" / "7z.exe";
                 std::string sevenZipCmd = "7z"; // Default to system PATH
                 if (std::filesystem::exists(bundled7z)) {
                     sevenZipCmd = "\"" + bundled7z.string() + "\"";
                     logger.log("Using bundled 7z: " + bundled7z.string());
                 } else {
                     logger.log("WARNING: No bundled 7z found, trying system PATH");
                 }
                 cmd = sevenZipCmd + " x \"" + sourcePath.string() + "\" -o\"" + gamesPath.string() + "\" -y";
             }
             
             // Append output redirection to capture errors
             if (!extractLogPath.empty()) {
                 cmd += " > \"" + extractLogPath.string() + "\" 2>&1";
             }

             logger.log("Executing: " + cmd);
             int result = system(cmd.c_str());
             
             if (result == 0) {
                 logger.log("Extraction successful for: " + filename);
                 archivesProcessed++;
                 
                 // Try to delete the archive after successful extraction
                 try {
                     std::filesystem::remove(sourcePath);
                     logger.log("Deleted archive: " + filename);
                 } catch (const std::exception& e) {
                     logger.log("WARNING: Could not delete archive: " + std::string(e.what()));
                 }
                 
                 romsFound++;
             } else {
                 logger.log("ERROR: Extraction failed with code: " + std::to_string(result) + " for: " + filename);
                 if (!extractLogPath.empty()) {
                     logger.log("See extraction log: " + extractLogPath.string());
                 }
                 if (result == 9009) {
                     logger.log("ERROR: Command not found (7z.exe not in PATH and not bundled)");
                 }
             }
        }
        else if (isRom) {
          std::filesystem::path sourcePath = downloadPath / filename;
          std::filesystem::path destPath = gamesPath / filename;
          
          logger.log("Found ROM file: " + filename);
          try {
              // Use filesystem rename (move) instead of system command
              // This is more robust and handles paths better
              std::filesystem::rename(sourcePath, destPath);
              logger.log("Moved to Games folder: " + filename);
              romsFound++;
          } catch (const std::filesystem::filesystem_error& e) {
              logger.log("ERROR: Failed to move " + filename + ": " + std::string(e.what()));
              // Try copy and delete as fallback (cross-device move)
              try {
                  std::filesystem::copy(sourcePath, destPath, std::filesystem::copy_options::overwrite_existing);
                  std::filesystem::remove(sourcePath);
                  logger.log("Copied and deleted (fallback): " + filename);
                  romsFound++;
              } catch (const std::exception& e2) {
                  logger.log("ERROR: Copy fallback failed: " + std::string(e2.what()));
              }
          }
        }
      } while (FindNextFileA(hFind, &findData) != 0);
      FindClose(hFind);
  }
  
  logger.log("Archives processed: " + std::to_string(archivesProcessed));
  logger.log("Total ROMs processed from Downloads: " + std::to_string(romsFound));
  
  // FIX 4: Recursively scan Games folder for ROMs (including subfolders from extracted archives)
  logger.log("Scanning Games folder recursively for ROMs...");
  int existingRoms = 0;
  
  try {
    for (const auto& entry : std::filesystem::recursive_directory_iterator(gamesPath)) {
      if (!entry.is_regular_file()) continue;
      
      std::string filename = entry.path().filename().string();
      std::string lower = filename;
      std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
      
      bool isRom = hasExtension(lower, ".iso") || hasExtension(lower, ".cso") || hasExtension(lower, ".pbp");
      
      if (isRom) {
        // Get the relative path from gamesPath for storage
        std::filesystem::path relativePath = std::filesystem::relative(entry.path(), gamesPath);
        std::string relativePathStr = relativePath.string();
        
        // Check if this ROM is already in the menu (avoid duplicates)
        bool alreadyExists = false;
        for (const auto& existingItem : categories_[gamesIdx].items) {
          // Check if path contains this filename
          if (existingItem.path.find(filename) != std::string::npos || 
              existingItem.path == relativePathStr) {
            alreadyExists = true;
            break;
          }
        }
        
        if (alreadyExists) {
          continue; // Skip this ROM, it's already in the menu
        }
        
        logger.log("Found ROM: " + relativePathStr);
        
        // Clean up display name
        std::string displayName = filename.substr(0, filename.find_last_of('.'));
        
        // Replace separators with spaces
        size_t pos;
        while ((pos = displayName.find(" - ")) != std::string::npos) {
            displayName.replace(pos, 3, " ");
        }
        std::replace(displayName.begin(), displayName.end(), '_', ' ');
        std::replace(displayName.begin(), displayName.end(), '-', ' ');

        // Remove region tags and language codes
        std::vector<std::string> tagsToRemove = {
          "(USA)", "(Europe)", "(Japan)", "(Asia)", "(World)", 
          "(En,Fr,De,Es,It)", "(En)", "(NTSC)", "(PAL)",
          "[USA]", "[Europe]", "[Japan]", "[Asia]", "[World]",
          "[!]", "[a]", "[b]", "[t]", "[f]", "[h]", "[o]"
        };
        
        for (const auto& tag : tagsToRemove) {
          while ((pos = displayName.find(tag)) != std::string::npos) {
            displayName.erase(pos, tag.length());
          }
        }
        
        // Remove anything in parentheses or brackets
        size_t start;
        while ((start = displayName.find('(')) != std::string::npos) {
          size_t end = displayName.find(')', start);
          if (end != std::string::npos) {
            displayName.erase(start, end - start + 1);
          } else {
            break;
          }
        }
        
        while ((start = displayName.find('[')) != std::string::npos) {
          size_t end = displayName.find(']', start);
          if (end != std::string::npos) {
            displayName.erase(start, end - start + 1);
          } else {
            break;
          }
        }
        
        // Replace underscores and hyphens with spaces
        std::replace(displayName.begin(), displayName.end(), '_', ' ');
        std::replace(displayName.begin(), displayName.end(), '-', ' ');
        
        // Trim leading/trailing spaces
        displayName.erase(0, displayName.find_first_not_of(" \t"));
        displayName.erase(displayName.find_last_not_of(" \t") + 1);
        
        // Remove multiple consecutive spaces
        while ((pos = displayName.find("  ")) != std::string::npos) {
          displayName.replace(pos, 2, " ");
        }
        
        // Add to menu
        MenuItem item;
        item.label = displayName.empty() ? filename : displayName;
        item.path = relativePathStr;  // Use relative path to support subfolders
        item.type = hasExtension(lower, ".pbp") ? "psp_eboot" : "psp_iso";
        item.iconFilename = "psp UMD.png";

        // Extract metadata from ROM - use full absolute path
        std::string fullPath = entry.path().string();
        try {
            // Ensure assets/previews exists
            std::filesystem::create_directories("assets/previews");
            CachedAssets assets = RomAssetManager::getOrExtractAssets(fullPath, "assets/previews");
            
            std::cout << "[Menu] Assets for " << filename << ": " << assets.iconPath << "\n";

            if (!assets.title.empty()) {
                item.label = assets.title;
            }
            
            if (!assets.iconPath.empty()) {
                item.previewImagePath = assets.iconPath;
                
                // Load for list icon
                if (item.iconTex.loadFromFile(assets.iconPath)) {
                    item.iconTex.setSmooth(true); // Enable smoothing
                    item.iconSprite.emplace(item.iconTex);
                    sf::Vector2u size = item.iconTex.getSize();
                    item.iconSprite->setOrigin({size.x / 2.f, size.y / 2.f});
                    item.iconSprite->setScale({0.5f, 0.5f});
                }

                // Load for preview card
                if (item.previewTexture.loadFromFile(assets.iconPath)) {
                    item.previewTexture.setSmooth(true); // Enable smoothing
                    item.previewSprite.emplace(item.previewTexture);
                    item.hasPreview = true;
                }
            }
            
            if (!assets.backgroundPath.empty()) {
                item.previewBgPath = assets.backgroundPath;
                item.coverArtPath = assets.backgroundPath;

                if (item.previewBgTexture.loadFromFile(assets.backgroundPath)) {
                    item.previewBgTexture.setSmooth(true);
                    item.previewBgSprite.emplace(item.previewBgTexture);
                    item.hasPreviewBg = true;
                }
                
                if (item.coverArtTexture.loadFromFile(assets.backgroundPath)) {
                    item.coverArtTexture.setSmooth(true);
                    item.coverArtSprite.emplace(item.coverArtTexture);
                    item.hasCoverArt = true;
                }
            }
            
            if (!assets.audioPath.empty()) {
                item.previewAudioPath = assets.audioPath;
            }

            std::cout << "Processed assets for " << filename << "\n";

        } catch (const std::exception& e) {
            std::cerr << "Failed to extract metadata for " << filename << ": " << e.what() << "\n";
        }
        
        categories_[gamesIdx].items.push_back(item);
        existingRoms++;
      }
    }
    logger.log("Found " + std::to_string(existingRoms) + " ROMs in Games folder (recursive scan)");
  } catch (const std::exception& e) {
    logger.log("ERROR: Failed to scan Games folder: " + std::string(e.what()));
  }
}

void Menu::handleEvent(const sf::Event& event) {
  // Handle keyboard input
  if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
    if (keyPressed->code == sf::Keyboard::Key::Left) {
      if (!categories_.empty()) {
        if (currentCategoryIndex_ == 0) {
          currentCategoryIndex_ = categories_.size() - 1;
        } else {
          --currentCategoryIndex_;
        }
        currentItemIndex_ = 0; // Reset item selection
        targetBgOffsetX_ += 15.f; // Move background right
        soundBank_.playCategoryDecide();
      }
    } else if (keyPressed->code == sf::Keyboard::Key::Right) {
      if (!categories_.empty()) {
        currentCategoryIndex_ = (currentCategoryIndex_ + 1) % categories_.size();
        currentItemIndex_ = 0; // Reset item selection
        targetBgOffsetX_ -= 15.f; // Move background left
        soundBank_.playCategoryDecide();
      }
    } else if (keyPressed->code == sf::Keyboard::Key::Up) {
      if (!categories_.empty() && !categories_[currentCategoryIndex_].items.empty()) {
        if (currentItemIndex_ == 0) {
          currentItemIndex_ = categories_[currentCategoryIndex_].items.size() - 1;
        } else {
          --currentItemIndex_;
        }
        targetBgOffsetY_ += 5.f; // Move background down slightly
        soundBank_.playCursor();
      }
    } else if (keyPressed->code == sf::Keyboard::Key::Down) {
      if (!categories_.empty() && !categories_[currentCategoryIndex_].items.empty()) {
        currentItemIndex_ = (currentItemIndex_ + 1) % categories_[currentCategoryIndex_].items.size();
        targetBgOffsetY_ -= 5.f; // Move background up slightly
        soundBank_.playCursor();
      }
    } else if (keyPressed->code == sf::Keyboard::Key::Enter) {
      if (!categories_.empty() && !categories_[currentCategoryIndex_].items.empty()) {
        launchRequested_ = true;
        soundBank_.playDecide();
      } else {
        soundBank_.playError();
      }
    } else if (keyPressed->code == sf::Keyboard::Key::Escape) {
      soundBank_.playCancel();
    }
  }
  
  // Handle gamepad/joystick button presses
  if (const auto* joystickButtonPressed = event.getIf<sf::Event::JoystickButtonPressed>()) {
    // Button 0 = Cross/A (confirm)
    if (joystickButtonPressed->button == 0) {
      if (!categories_.empty() && !categories_[currentCategoryIndex_].items.empty()) {
        launchRequested_ = true;
        soundBank_.playDecide();
      } else {
        soundBank_.playError();
      }
    }
    // Button 1 = Circle/B (back)
    else if (joystickButtonPressed->button == 1) {
      soundBank_.playCancel();
    }
  }
  
  // Handle D-pad
  if (const auto* joystickMoved = event.getIf<sf::Event::JoystickMoved>()) {
    const float deadzone = 50.f;
    
    // Horizontal axis (D-pad left/right or left stick)
    if (joystickMoved->axis == sf::Joystick::Axis::X || joystickMoved->axis == sf::Joystick::Axis::PovX) {
      if (joystickMoved->position < -deadzone) {
        // Left
        if (!categories_.empty()) {
          if (currentCategoryIndex_ == 0) {
            currentCategoryIndex_ = categories_.size() - 1;
          } else {
            --currentCategoryIndex_;
          }
          currentItemIndex_ = 0;
          targetBgOffsetX_ += 15.f;
          soundBank_.playCategoryDecide();
        }
      } else if (joystickMoved->position > deadzone) {
        // Right
        if (!categories_.empty()) {
          currentCategoryIndex_ = (currentCategoryIndex_ + 1) % categories_.size();
          currentItemIndex_ = 0;
          targetBgOffsetX_ -= 15.f;
          soundBank_.playCategoryDecide();
        }
      }
    }
    
    // Vertical axis (D-pad up/down or left stick)
    if (joystickMoved->axis == sf::Joystick::Axis::Y || joystickMoved->axis == sf::Joystick::Axis::PovY) {
      if (joystickMoved->position > deadzone) {
        // Up (inverted)
        if (!categories_.empty() && !categories_[currentCategoryIndex_].items.empty()) {
          if (currentItemIndex_ == 0) {
            currentItemIndex_ = categories_[currentCategoryIndex_].items.size() - 1;
          } else {
            --currentItemIndex_;
          }
          targetBgOffsetY_ += 5.f;
          soundBank_.playCursor();
        }
      } else if (joystickMoved->position < -deadzone) {
        // Down (inverted)
        if (!categories_.empty() && !categories_[currentCategoryIndex_].items.empty()) {
          currentItemIndex_ = (currentItemIndex_ + 1) % categories_[currentCategoryIndex_].items.size();
          targetBgOffsetY_ -= 5.f;
          soundBank_.playCursor();
        }
      }
    }
  }
}

void Menu::update(float dt) {
  // Check for selection change to update preview audio
  if (currentCategoryIndex_ != lastCategoryIndex_ || currentItemIndex_ != lastItemIndex_) {
    lastCategoryIndex_ = currentCategoryIndex_;
    lastItemIndex_ = currentItemIndex_;
    
    // Stop previous sound
    if (previewSoundPlayer_) previewSoundPlayer_->stop();
    
    // Play new sound if available
    if (!categories_.empty() && !categories_[currentCategoryIndex_].items.empty()) {
        const auto& item = categories_[currentCategoryIndex_].items[currentItemIndex_];
        if (item.hasPreviewAudio && item.previewBuffer) {
            previewSoundPlayer_.emplace(*item.previewBuffer);
            previewSoundPlayer_->setLooping(true);
            previewSoundPlayer_->play();
        }
    }
    
    // Reset preview alpha on selection change
    previewAlpha_ = 0.f;
  }
  
  // Update preview alpha
  previewAlpha_ += 600.f * dt; // Fast fade in
  if (previewAlpha_ > 255.f) previewAlpha_ = 255.f;

  // Smooth scroll animation for item list
  const float lerpSpeed = 8.0f;
  targetItemListOffset_ = -static_cast<float>(currentItemIndex_) * ITEM_ROW_HEIGHT;
  itemListOffset_ += (targetItemListOffset_ - itemListOffset_) * lerpSpeed * dt;

  // Category scroll animation (Parallax/Carousel)
  targetCategoryScrollOffset_ = static_cast<float>(currentCategoryIndex_);
  
  // Handle wrap-around smoothing
  // If we are far apart (more than half the list), we might want to snap or handle it differently.
  // For now, simple lerp. To fix the "long scroll" on wrap, we'd need virtual indices.
  // A simple fix for visual comfort: if distance is large, snap immediately or increase speed.
  float diff = targetCategoryScrollOffset_ - categoryScrollOffset_;
  if (std::abs(diff) > categories_.size() / 2.0f) {
      // We wrapped around. Snap to target to avoid dizzying scroll.
      categoryScrollOffset_ = targetCategoryScrollOffset_;
  } else {
      categoryScrollOffset_ += (targetCategoryScrollOffset_ - categoryScrollOffset_) * lerpSpeed * dt;
  }

  // Category scale animation (subtle pulse on selected)
  categoryScaleAnim_ += dt * 2.0f;

  // Parallax background animation with spring back
  const float bgLerpSpeed = 5.0f;
  const float bgSpringBack = 3.0f;
  
  // Smooth movement toward target
  bgOffsetX_ += (targetBgOffsetX_ - bgOffsetX_) * bgLerpSpeed * dt;
  bgOffsetY_ += (targetBgOffsetY_ - bgOffsetY_) * bgLerpSpeed * dt;
  
  // Spring back toward center
  targetBgOffsetX_ -= targetBgOffsetX_ * bgSpringBack * dt;
  targetBgOffsetY_ -= targetBgOffsetY_ * bgSpringBack * dt;
  
  // Clamp to prevent excessive movement
  bgOffsetX_ = std::max(-20.f, std::min(20.f, bgOffsetX_));
  bgOffsetY_ = std::max(-20.f, std::min(20.f, bgOffsetY_));
}

sf::Vector2f Menu::getCategoryIconPosition(size_t index) const {
  if (categories_.empty()) return {0.f, 0.f};

  // Parallax/Carousel positioning
  // Center the selected category (defined by categoryScrollOffset_)
  const float centerX = 1280.f / 2.f;
  
  // Calculate position relative to the scroll offset
  // We use the interpolated categoryScrollOffset_ for smooth movement
  float x = centerX + (static_cast<float>(index) - categoryScrollOffset_) * CATEGORY_SPACING;
  
  return {x, CATEGORY_Y_POS};
}

std::string Menu::getItemTypeDisplay(const std::string& type) const {
  if (type == "psp_iso") return "PSP ISO";
  if (type == "psp_eboot") return "PSP Homebrew";
  if (type == "pc_app") return "PC Application";
  if (type == "web_url") return "Web Link";
  return "Unknown";
}

void Menu::draw(sf::RenderWindow& window) {
  // 1. Background with parallax offset
  if (bgLoaded_ && bgSprite_.has_value()) {
    bgSprite_->setPosition({-20.f + bgOffsetX_, -20.f + bgOffsetY_});
    window.draw(*bgSprite_);
  } else {
    window.clear(sf::Color(5, 5, 5)); // Very dark gray
  }

  if (!fontLoaded_) return;

  // 2. Top bar
  // Title with username
  std::string titleStr = "PSPV2";
  if (userProfile_ && !userProfile_->getUserName().empty()) {
    titleStr = "Welcome, " + userProfile_->getUserName();
  }
  sf::Text titleText(font_, titleStr, 24);
  titleText.setFillColor(sf::Color::White);
  titleText.setPosition({20.f, 20.f});
  window.draw(titleText);

  // Date and Time (if enabled in profile)
  std::time_t now = std::time(nullptr);
  std::tm* localTime = std::localtime(&now);
  
  std::ostringstream dateTimeStream;
  
  // Show date if enabled
  if (userProfile_ && userProfile_->getShowDate()) {
    dateTimeStream << std::setfill('0') << std::setw(2) << (localTime->tm_mon + 1) << "/"
                   << std::setfill('0') << std::setw(2) << localTime->tm_mday << "/"
                   << (localTime->tm_year + 1900) << "  ";
  }
  
  // Show time if enabled
  if (userProfile_ && userProfile_->getShowClock()) {
    if (userProfile_->getUse24HourFormat()) {
      // 24-hour format
      dateTimeStream << std::setfill('0') << std::setw(2) << localTime->tm_hour << ":"
                     << std::setfill('0') << std::setw(2) << localTime->tm_min;
    } else {
      // 12-hour format with AM/PM
      int hour12 = localTime->tm_hour % 12;
      if (hour12 == 0) hour12 = 12; // Convert 0 to 12
      dateTimeStream << hour12 << ":"
                     << std::setfill('0') << std::setw(2) << localTime->tm_min << " "
                     << (localTime->tm_hour < 12 ? "AM" : "PM");
    }
  }
  
  // Battery icon (positioned before date/time to avoid overlap)
  sf::RectangleShape batteryOutline({40.f, 18.f});
  batteryOutline.setPosition({1000.f, 24.f});
  batteryOutline.setFillColor(sf::Color::Transparent);
  batteryOutline.setOutlineColor(sf::Color::White);
  batteryOutline.setOutlineThickness(2.f);
  window.draw(batteryOutline);
  
  sf::RectangleShape batteryFill({32.f, 10.f});
  batteryFill.setPosition({1004.f, 28.f});
  batteryFill.setFillColor(sf::Color::White);
  window.draw(batteryFill);

  sf::RectangleShape batteryTip({4.f, 10.f});
  batteryTip.setPosition({1040.f, 29.f});
  batteryTip.setFillColor(sf::Color::White);
  window.draw(batteryTip);

  sf::Text clockText(font_, dateTimeStream.str(), 24);
  clockText.setFillColor(sf::Color::White);
  sf::FloatRect clockBounds = clockText.getLocalBounds();
  clockText.setPosition({1260.f - clockBounds.size.x, 20.f});
  window.draw(clockText);

  if (categories_.empty()) {
    sf::Text emptyText(font_, "No categories loaded", 32);
    emptyText.setFillColor(sf::Color::White);
    emptyText.setPosition({500.f, 360.f});
    window.draw(emptyText);
    return;
  }

  // Draw preview background and elements (behind UI)
  {
      const auto& currentCat = categories_[currentCategoryIndex_];
      if (!currentCat.items.empty()) {
          const auto& selectedItem = currentCat.items[currentItemIndex_];
          
          if (selectedItem.type == "psp_iso" || selectedItem.type == "psp_eboot") {
              // 1) Background wallpaper (Fullscreen - Cover Mode)
              if (selectedItem.hasPreviewBg && selectedItem.previewBgSprite) {
                  sf::Sprite bg = *selectedItem.previewBgSprite;
                  
                  sf::Vector2u windowSize = window.getSize();
                  auto texSize = selectedItem.previewBgTexture.getSize();
                  
                  // Calculate scale to COVER the screen (max of X and Y scales)
                  // This ensures the image fills the entire screen without distortion
                  float scaleX = static_cast<float>(windowSize.x) / texSize.x;
                  float scaleY = static_cast<float>(windowSize.y) / texSize.y;
                  float scale = std::max(scaleX, scaleY);
                  
                  bg.setScale({scale, scale});
                  
                  // Center the sprite perfectly
                  bg.setOrigin({texSize.x / 2.f, texSize.y / 2.f});
                  bg.setPosition({windowSize.x / 2.f, windowSize.y / 2.f});
                  
                  bg.setColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(std::min(255.f, previewAlpha_))));
                  window.draw(bg);
              }

              // 2) Preview “video window”
              if (selectedItem.hasPreview) {
                  sf::Sprite preview = *selectedItem.previewSprite;

                  float targetW = 320.f;
                  float targetH = 180.f;
                  auto texSize = selectedItem.previewTexture.getSize();
                  float scaleX = targetW / texSize.x;
                  float scaleY = targetH / texSize.y;
                  float scale  = std::min(scaleX, scaleY);
                  preview.setScale({scale, scale});

                  // Move to top-right area
                  float px = 880.f; 
                  float py = 80.f;
                  preview.setPosition({px, py});
                  
                  sf::Color fadeColor(255, 255, 255, static_cast<std::uint8_t>(previewAlpha_));
                  preview.setColor(fadeColor);

                  // subtle drop shadow
                  sf::RectangleShape shadow({targetW + 12.f, targetH + 12.f});
                  shadow.setPosition({px - 6.f + 4.f, py - 6.f + 4.f});
                  shadow.setFillColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(std::min(150.f, previewAlpha_))));
                  window.draw(shadow);

                  // white border
                  sf::RectangleShape frame({targetW + 8.f, targetH + 8.f});
                  frame.setPosition({px - 4.f, py - 4.f});
                  frame.setFillColor(sf::Color(230, 230, 230, static_cast<std::uint8_t>(std::min(230.f, previewAlpha_))));
                  window.draw(frame);

                  window.draw(preview);
              }

              // 3) Big cover art
              if (selectedItem.hasCoverArt) {
                  sf::Sprite cover = *selectedItem.coverArtSprite;

                  float targetW = 200.f; // Slightly smaller to fit better
                  float targetH = 320.f;
                  auto texSize = selectedItem.coverArtTexture.getSize();
                  float scaleX = targetW / texSize.x;
                  float scaleY = targetH / texSize.y;
                  float scale  = std::min(scaleX, scaleY);
                  cover.setScale({scale, scale});

                  // Position below the video preview
                  float cx = 940.f; 
                  float cy = 300.f; 
                  cover.setPosition({cx, cy});
                  
                  sf::Color fadeColor(255, 255, 255, static_cast<std::uint8_t>(previewAlpha_));
                  cover.setColor(fadeColor);

                  // small glow
                  sf::RectangleShape glow({targetW + 20.f, targetH + 20.f});
                  glow.setPosition({cx - 10.f, cy - 10.f});
                  glow.setFillColor(sf::Color::Transparent);
                  glow.setOutlineColor(sf::Color(0, 220, 255, static_cast<std::uint8_t>(std::min(100.f, previewAlpha_))));
                  glow.setOutlineThickness(4.f);
                  window.draw(glow);

                  window.draw(cover);
              }
          }
      }
  }

  // 3. Category icons (Parallax Scroll)
  for (size_t i = 0; i < categories_.size(); ++i) {
    auto& cat = categories_[i];
    sf::Vector2f pos = getCategoryIconPosition(i);

    // Skip if off-screen to save performance
    if (pos.x < -100.f || pos.x > 1380.f) continue;

    // Calculate distance from center for parallax scaling
    float dist = std::abs(pos.x - 640.f);
    float scaleRatio = std::max(0.f, 1.f - (dist / 600.f)); // 0 to 1 based on proximity to center
    
    // Smooth easing for scale ratio
    scaleRatio = scaleRatio * scaleRatio * (3.f - 2.f * scaleRatio);

    bool isSelected = (i == currentCategoryIndex_);
    
    if (cat.iconSprite.has_value()) {
      float targetSize = CATEGORY_ICON_SIZE + (CATEGORY_ICON_SIZE_SELECTED - CATEGORY_ICON_SIZE) * scaleRatio;
      float scale = targetSize / cat.iconTex.getSize().x;
      
      // Add subtle pulse to selected
      if (isSelected) {
        scale *= 1.0f + 0.05f * std::sin(categoryScaleAnim_);
      }

      cat.iconSprite->setPosition(pos);
      cat.iconSprite->setScale({scale, scale});
      
      // Fade out distant icons
      sf::Color iconColor = sf::Color::White;
      iconColor.a = static_cast<std::uint8_t>(50 + 205 * scaleRatio);
      cat.iconSprite->setColor(iconColor);
      
      window.draw(*cat.iconSprite);
    }

    // Category label
    // Only show label if it's close to center or selected
    if (scaleRatio > 0.5f) {
        sf::Text catLabel(font_, cat.label, static_cast<unsigned int>(16 + 8 * scaleRatio));
        
        sf::Color labelColor = isSelected ? sf::Color::White : sf::Color(180, 180, 180);
        labelColor.a = static_cast<std::uint8_t>(255 * scaleRatio);
        catLabel.setFillColor(labelColor);
        
        sf::FloatRect bounds = catLabel.getLocalBounds();
        catLabel.setOrigin({bounds.size.x / 2.f, 0.f});
        
        // Position label below icon, adjusting for icon size
        float yOffset = (CATEGORY_ICON_SIZE + (CATEGORY_ICON_SIZE_SELECTED - CATEGORY_ICON_SIZE) * scaleRatio) / 2.f;
        catLabel.setPosition({pos.x, pos.y + yOffset + 20.f});
        
        window.draw(catLabel);
    }
  }

  // 4. Item list for current category
  const auto& currentCat = categories_[currentCategoryIndex_];
  
  if (currentCat.items.empty()) {
    sf::Text emptyText(font_, "No items in this category", 24);
    emptyText.setFillColor(sf::Color(150, 150, 150));
    emptyText.setPosition({400.f, ITEM_LIST_START_Y + 50.f});
    window.draw(emptyText);
  } else {
    // Draw items with smooth scrolling
    for (size_t i = 0; i < currentCat.items.size(); ++i) {
      auto& item = currentCat.items[i];
      bool isSelected = (i == currentItemIndex_);
      
      float yPos = ITEM_LIST_START_Y + i * ITEM_ROW_HEIGHT + itemListOffset_;
      
      // Selection highlight
      if (isSelected) {
        sf::RectangleShape highlight({500.f, ITEM_ROW_HEIGHT - 4.f});
        highlight.setPosition({250.f, yPos});
        highlight.setFillColor(sf::Color(255, 255, 255, 30));
        highlight.setOutlineColor(sf::Color(0, 255, 255, 150));
        highlight.setOutlineThickness(2.f);
        window.draw(highlight);
      }

      // Item icon (if available)
      float textXPos = 270.f;
      if (item.iconSprite.has_value()) {
        float iconSize = isSelected ? 28.f : 24.f;
        float iconScale = iconSize / item.iconTex.getSize().x;
        sf::Sprite iconSprite(item.iconTex);
        iconSprite.setScale({iconScale, iconScale});
        iconSprite.setPosition({260.f, yPos + ITEM_ROW_HEIGHT / 2.f});
        iconSprite.setOrigin({item.iconTex.getSize().x / 2.f, item.iconTex.getSize().y / 2.f});
        window.draw(iconSprite);
        textXPos = 295.f; // Shift text to the right to make room for icon
      }
      
      // Item text
      sf::Text itemText(font_, item.label, isSelected ? 22 : 18);
      itemText.setFillColor(isSelected ? sf::Color::White : sf::Color(200, 200, 200));
      itemText.setPosition({textXPos, yPos + 8.f});
      window.draw(itemText);
    }
  }

  // 5. Info panel on right side
  if (!currentCat.items.empty()) {
    const auto& selectedItem = currentCat.items[currentItemIndex_];
    
    float infoPanelX = 820.f;
    float infoPanelY = ITEM_LIST_START_Y;

    // Move text below images for games
    if (selectedItem.type == "psp_iso" || selectedItem.type == "psp_eboot") {
        infoPanelX = 880.f;
        infoPanelY = 640.f;
    }

    // Selected item label
    sf::Text itemTitleText(font_, selectedItem.label, 28);
    itemTitleText.setFillColor(sf::Color::White);
    itemTitleText.setPosition({infoPanelX, infoPanelY});
    window.draw(itemTitleText);

    // Item type - show current setting for toggle_time_format
    std::string typeDisplay = getItemTypeDisplay(selectedItem.type);
    if (selectedItem.type == "toggle_time_format" && userProfile_) {
      typeDisplay = userProfile_->getUse24HourFormat() ? "Currently: 24-Hour" : "Currently: 12-Hour";
    }
    sf::Text itemTypeText(font_, typeDisplay, 18);
    itemTypeText.setFillColor(sf::Color(180, 180, 180));
    itemTypeText.setPosition({infoPanelX, infoPanelY + 40.f});
    window.draw(itemTypeText);

    // Path (truncated if too long)
    std::string pathDisplay = selectedItem.path;
    if (pathDisplay.length() > 40) {
      pathDisplay = "..." + pathDisplay.substr(pathDisplay.length() - 37);
    }
    sf::Text itemPathText(font_, pathDisplay, 14);
    itemPathText.setFillColor(sf::Color(120, 120, 120));
    itemPathText.setPosition({infoPanelX, infoPanelY + 70.f});
    window.draw(itemPathText);
  }

  // 6. Bottom control hints
  sf::Text hintsText(font_, "Left/Right: Category  |  Up/Down: Select  |  Enter: Launch  |  Esc: Exit", 16);
  hintsText.setFillColor(sf::Color(150, 150, 150));
  hintsText.setPosition({30.f, 680.f});
  window.draw(hintsText);
}

MenuItem Menu::getSelectedItem() const {
  if (categories_.empty()) return MenuItem{"", "", "pc_app"};
  const auto& cat = categories_[currentCategoryIndex_];
  if (cat.items.empty()) return MenuItem{"", "", "pc_app"};
  return cat.items[currentItemIndex_];
}
