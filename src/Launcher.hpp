#pragma once

#include "Menu.hpp"
#include <string>

class Launcher {
public:
  Launcher(const std::string& settingsPath);
  void launchItem(const MenuItem& item, bool useController = false);

private:
  std::string ppssppPath_;
  std::string gamesRoot_;
  bool emulatorFullscreen_;
};
