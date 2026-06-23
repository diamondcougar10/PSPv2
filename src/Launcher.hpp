#pragma once

#include "Menu.hpp"
#include <string>

class Launcher {
public:
  Launcher(const std::string& settingsPath);
  void launchItem(const MenuItem& item, bool useController = false);

private:
  void ensurePPSSPPDirectories();
  
  std::string ppssppPath_;
  std::string gamesRoot_;
  std::string ppssppMemstickRoot_;
  std::string ppssppSavedataPath_;
  std::string ppssppSystemPath_;
  bool emulatorFullscreen_;
};
