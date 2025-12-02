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

using json = nlohmann::json;

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
  // Helper lambda to check file extension (C++17 compatible)
  auto hasExtension = [](const std::string& filename, const std::string& ext) {
    if (filename.length() < ext.length()) return false;
    return filename.compare(filename.length() - ext.length(), ext.length(), ext) == 0;
  };
  
  // Scan Downloads folder for PSP ROMs
  std::string downloadPath = "Downloads";
  std::string gamesPath = gamesRoot_;  // Use configured games root path
  std::string searchPath = downloadPath + "/*.*";
  
  WIN32_FIND_DATAA findData;
  HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
  
  if (hFind == INVALID_HANDLE_VALUE) {
    std::cout << "No files in Downloads folder\n";
    return;
  }
  
  // Find the Games category
  size_t gamesIdx = 0;
  for (size_t i = 0; i < categories_.size(); ++i) {
    if (categories_[i].id == "games") {
      gamesIdx = i;
      break;
    }
  }
  
  // First pass: Extract any zip files
  std::vector<std::string> zipFiles;
  do {
    std::string filename = findData.cFileName;
    if (filename == "." || filename == "..") continue;
    
    std::string lower = filename;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    if (hasExtension(lower, ".zip")) {
      zipFiles.push_back(filename);
    }
  } while (FindNextFileA(hFind, &findData) != 0);
  FindClose(hFind);
  
  // Extract zip files using PowerShell
  for (const auto& zipFile : zipFiles) {
    std::string zipPath = downloadPath + "\\" + zipFile;
    std::string extractPath = downloadPath;
    
    std::cout << "Extracting: " << zipFile << "...\n";
    
    // Use PowerShell Expand-Archive to extract
    std::string cmd = "powershell -Command \"Expand-Archive -Path '" + zipPath + 
                     "' -DestinationPath '" + extractPath + "' -Force\"";
    int result = std::system(cmd.c_str());
    
    if (result == 0) {
      std::cout << "Extracted successfully: " << zipFile << "\n";
      
      // Delete the zip file after extraction
      std::string deleteCmd = "del /Q \"" + zipPath + "\"";
      std::system(deleteCmd.c_str());
      std::cout << "Deleted zip file: " << zipFile << "\n";
    } else {
      std::cerr << "Failed to extract: " << zipFile << "\n";
    }
  }
  
  // Second pass: Move ROM files from Downloads to Games folder
  hFind = FindFirstFileA(searchPath.c_str(), &findData);
  if (hFind == INVALID_HANDLE_VALUE) return;
  
  int romsFound = 0;
  do {
    std::string filename = findData.cFileName;
    if (filename == "." || filename == "..") continue;
    
    // Check if it's a PSP ROM file (.iso, .cso, .pbp)
    std::string lower = filename;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    bool isRom = hasExtension(lower, ".iso") || hasExtension(lower, ".cso") || hasExtension(lower, ".pbp");
    
    if (isRom) {
      std::string sourcePath = downloadPath + "\\" + filename;
      std::string destPath = gamesPath + "\\" + filename;
      
      // Move the ROM file to Games folder
      std::string moveCmd = "move /Y \"" + sourcePath + "\" \"" + destPath + "\"";
      int moveResult = std::system(moveCmd.c_str());
      
      if (moveResult == 0) {
        std::cout << "Moved to Games folder: " << filename << "\n";
        
        // Clean up display name - remove extension first
        std::string displayName = filename.substr(0, filename.find_last_of('.'));
        
        // Replace separators with spaces
        size_t pos;
        while ((pos = displayName.find(" - ")) != std::string::npos) {
            displayName.replace(pos, 3, " ");
        }
        std::replace(displayName.begin(), displayName.end(), '_', ' ');
        
        // Remove common ROM tags and region codes
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
        
        // Replace underscores with spaces and trim
        std::replace(displayName.begin(), displayName.end(), '_', ' ');
        
        // Trim leading/trailing spaces
        displayName.erase(0, displayName.find_first_not_of(" \t"));
        displayName.erase(displayName.find_last_not_of(" \t") + 1);
        
        // Remove multiple consecutive spaces
        while ((pos = displayName.find("  ")) != std::string::npos) {
          displayName.replace(pos, 2, " ");
        }
        
        // We used to add to menu here, but now we let the second pass handle it
        // so that metadata extraction logic is centralized.
        romsFound++;
      } else {
        std::cerr << "Failed to move: " << filename << "\n";
      }
    }
  } while (FindNextFileA(hFind, &findData) != 0);
  
  FindClose(hFind);
  std::cout << "Total ROMs processed from Downloads: " << romsFound << "\n";
  
  // Also scan Games folder for existing ROMs
  searchPath = gamesPath + "/*.*";
  hFind = FindFirstFileA(searchPath.c_str(), &findData);
  
  if (hFind != INVALID_HANDLE_VALUE) {
    int existingRoms = 0;
    do {
      std::string filename = findData.cFileName;
      if (filename == "." || filename == "..") continue;
      
      std::string lower = filename;
      std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
      
      bool isRom = hasExtension(lower, ".iso") || hasExtension(lower, ".cso") || hasExtension(lower, ".pbp");
      
      if (isRom) {
        // Check if this ROM is already in the menu (avoid duplicates)
        bool alreadyExists = false;
        for (const auto& existingItem : categories_[gamesIdx].items) {
          // Check if path contains this filename
          if (existingItem.path.find(filename) != std::string::npos) {
            alreadyExists = true;
            break;
          }
        }
        
        if (alreadyExists) {
          continue; // Skip this ROM, it's already in the menu
        }
        
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
        item.path = filename;  // Just filename, launcher will prepend gamesRoot
        item.type = hasExtension(lower, ".pbp") ? "psp_eboot" : "psp_iso";
        item.iconFilename = "psp UMD.png";

        // Extract metadata from ROM
        std::string fullPath = gamesPath + "\\" + filename;
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
                    item.iconSprite.emplace(item.iconTex);
                    sf::Vector2u size = item.iconTex.getSize();
                    item.iconSprite->setOrigin({size.x / 2.f, size.y / 2.f});
                    item.iconSprite->setScale({0.5f, 0.5f});
                }

                // Load for preview card
                if (item.previewTexture.loadFromFile(assets.iconPath)) {
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
    } while (FindNextFileA(hFind, &findData) != 0);
    
    FindClose(hFind);
    std::cout << "Found " << existingRoms << " existing ROMs in Games folder\n";
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

  const float totalWidth = 1280.f;
  const float spacing = 120.f;
  const float totalCatWidth = categories_.size() * spacing;
  const float startX = (totalWidth - totalCatWidth) / 2.f + spacing / 2.f;

  return {startX + index * spacing, CATEGORY_Y_POS};
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

  // 3. Category icons
  for (size_t i = 0; i < categories_.size(); ++i) {
    auto& cat = categories_[i];
    sf::Vector2f pos = getCategoryIconPosition(i);

    bool isSelected = (i == currentCategoryIndex_);
    
    if (cat.iconSprite.has_value()) {
      float scale = isSelected ? (CATEGORY_ICON_SIZE_SELECTED / cat.iconTex.getSize().x) 
                               : (CATEGORY_ICON_SIZE / cat.iconTex.getSize().x);
      
      // Add subtle pulse to selected
      if (isSelected) {
        scale *= 1.0f + 0.05f * std::sin(categoryScaleAnim_);
      }

      cat.iconSprite->setPosition(pos);
      cat.iconSprite->setScale({scale, scale});
      window.draw(*cat.iconSprite);
    }

    // Category label
    sf::Text catLabel(font_, cat.label, isSelected ? 20 : 16);
    catLabel.setFillColor(isSelected ? sf::Color::White : sf::Color(180, 180, 180));
    sf::FloatRect bounds = catLabel.getLocalBounds();
    catLabel.setOrigin({bounds.size.x / 2.f, 0.f});
    catLabel.setPosition({pos.x, pos.y + (isSelected ? 50.f : 40.f)});
    window.draw(catLabel);
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
