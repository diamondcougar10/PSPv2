#include "Launcher.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <filesystem>

using json = nlohmann::json;
namespace fs = std::filesystem;

Launcher::Launcher(const std::string& settingsPath) {
  std::ifstream ifs(settingsPath);
  if (!ifs) {
    std::cerr << "Failed to open settings file: " << settingsPath << "\n";
    return;
  }

  try {
    json j;
    ifs >> j;
    ppssppPath_ = j.value("ppsspp_path", std::string(""));
    gamesRoot_ = j.value("games_root", std::string(""));
    ppssppMemstickRoot_ = j.value("ppsspp_memstick_root", std::string(""));
    ppssppSavedataPath_ = j.value("ppsspp_savedata_path", std::string(""));
    ppssppSystemPath_ = j.value("ppsspp_system_path", std::string(""));
    emulatorFullscreen_ = j.value("emulator_fullscreen", true);
    
    if (ppssppPath_.empty()) {
      std::cerr << "Warning: ppsspp_path is missing in settings.json\n";
    }
    if (gamesRoot_.empty()) {
      std::cerr << "Warning: games_root is missing in settings.json\n";
    }
    
    // Ensure PPSSPP directories exist
    ensurePPSSPPDirectories();
  } catch (const std::exception& e) {
    std::cerr << "Error parsing settings.json: " << e.what() << "\n";
  }
}

void Launcher::launchItem(const MenuItem& item, bool useController) {
  if (item.type == "psp_iso" || item.type == "psp_eboot") {
    if (ppssppPath_.empty()) {
      std::cerr << "PPSSPP path is not set in settings.json\n";
      return;
    }
    
    if (item.path.empty()) {
      std::cerr << "Item path is empty\n";
      return;
    }
    
    // Build full game path
    fs::path gamePath(item.path);
    
    // If path is not absolute, treat it as relative to gamesRoot
    if (!gamePath.is_absolute() && !gamesRoot_.empty()) {
      gamePath = fs::path(gamesRoot_) / gamePath;
    }
    
    // Convert to Windows-style backslashes
    std::string gamePathStr = gamePath.make_preferred().string();
    std::string ppssppPathWin = fs::path(ppssppPath_).make_preferred().string();
    
    // Build command - put game path first, then flags
    std::string cmd = "cmd /c start \"\" \"" + ppssppPathWin + "\" \"" + gamePathStr + "\"";
    
    if (emulatorFullscreen_) {
      cmd += " --fullscreen";
    }
    
    // Add memstick root path if configured (PPSSPP will use this for saves/system data)
    if (!ppssppMemstickRoot_.empty()) {
      cmd += " --memstick=\"" + ppssppMemstickRoot_ + "\"";
      std::cout << "Using memstick root: " << ppssppMemstickRoot_ << "\n";
    }
    
    // Configure input method
    if (useController) {
      std::cout << "Launching with PS5 Controller support\n";
    } else {
      std::cout << "Launching with Keyboard & Mouse\n";
    }
    
    cmd += "\"";
    
    std::cout << "Launching PPSSPP: " << cmd << "\n";
    int result = std::system(cmd.c_str());
    
    if (result != 0) {
      std::cerr << "PPSSPP exited with code " << result << "\n";
    }

  } else if (item.type == "pc_app") {
    std::string cmd = "\"" + item.path + "\"";
    std::cout << "Launching PC app: " << cmd << "\n";
    std::system(cmd.c_str());
  } else if (item.type == "folder") {
    // Open folder in Windows Explorer
    std::string cmd = "explorer \"" + item.path + "\"";
    std::cout << "Opening folder: " << item.path << "\n";
    std::system(cmd.c_str());
  } else if (item.type == "web_url") {
    // Open URL in default browser
    std::string cmd = "start \"\" \"" + item.path + "\"";
    std::cout << "Opening URL: " << item.path << "\n";
    std::system(cmd.c_str());
  } else {
    std::cerr << "Unknown item type: " << item.type << "\n";
  }
}

void Launcher::ensurePPSSPPDirectories() {
  // Create PPSSPP directory structure to mimic PSP console
  if (!ppssppMemstickRoot_.empty()) {
    fs::path memstickPath(ppssppMemstickRoot_);
    
    // Create main directories
    std::vector<std::string> directories = {
      "SAVEDATA",
      "SYSTEM",
      "GAME",
      "PSP/SAVEDATA",
      "PSP/SYSTEM",
      "PSP/GAME",
      "PSP/COMMON",
      "ISO",
      "MUSIC",
      "PHOTO",
      "VIDEO"
    };
    
    for (const auto& dir : directories) {
      fs::path fullPath = memstickPath / dir;
      try {
        if (!fs::exists(fullPath)) {
          fs::create_directories(fullPath);
          std::cout << "Created PPSSPP directory: " << fullPath << "\n";
        }
      } catch (const std::exception& e) {
        std::cerr << "Failed to create directory " << fullPath << ": " << e.what() << "\n";
      }
    }
    
    // Create a system.txt file with console info
    fs::path systemFile = memstickPath / "PSP" / "SYSTEM" / "system.txt";
    if (!fs::exists(systemFile)) {
      std::ofstream sysFile(systemFile);
      if (sysFile) {
        sysFile << "PSP Virtual Console v2.0\n";
        sysFile << "Emulated PSP System\n";
        sysFile << "Connected to PPSSPP\n";
        sysFile.close();
        std::cout << "Created system info file\n";
      }
    }
  }
}
