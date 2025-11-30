#include "Launcher.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <filesystem>
#include <algorithm>

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
    emulatorFullscreen_ = j.value("emulator_fullscreen", true);
    
    if (ppssppPath_.empty()) {
      std::cerr << "Warning: ppsspp_path is missing in settings.json\n";
    }
    if (gamesRoot_.empty()) {
      std::cerr << "Warning: games_root is missing in settings.json\n";
    }
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
    
    // Configure input method
    if (useController) {
      std::cout << "Launching with PS5 Controller support\n";
    } else {
      std::cout << "Launching with Keyboard & Mouse\n";
    }
    
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
    // Convert forward slashes to backslashes for Windows paths
    std::string normalizedPath = item.path;
    std::replace(normalizedPath.begin(), normalizedPath.end(), '/', '\\');
    
    // Use /select, flag for specific folders to ensure correct path
    std::string cmd = "explorer \"" + normalizedPath + "\"";
    std::cout << "Opening folder: " << normalizedPath << "\n";
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
