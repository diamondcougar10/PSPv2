#include <SFML/Graphics.hpp>
#include "Menu.hpp"
#include "Launcher.hpp"
#include "IntroScreen.hpp"
#include "GameStartupScreen.hpp"
#include "ControllerSelectScreen.hpp"
#include "SetupScreen.hpp"
#include "UserProfile.hpp"
#include "ThemeSelector.hpp"
#include "CustomThemeCreator.hpp"
#include "AboutScreen.hpp"
#include "UiSoundBank.hpp"
#include "QuickMenu.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>

using json = nlohmann::json;

enum class AppState {
    Setup,
    Intro,
    Menu,
    ControllerSelect,
    GameStartup,
    ThemeSelect,
    ThemeCreator,
    About
};

int main() {
  // Load settings for fullscreen mode
  bool fullscreen = true;  // Default to fullscreen
  std::ifstream settingsFile("config/settings.json");
  if (settingsFile) {
    try {
      json settingsJson;
      settingsFile >> settingsJson;
      fullscreen = settingsJson.value("fullscreen", true);
    } catch (...) {
      // Use default if parsing fails
    }
  }

  sf::RenderWindow window;
  sf::Vector2u windowSize;
  if (fullscreen) {
    window.create(sf::VideoMode::getDesktopMode(), "PSPV2", sf::State::Fullscreen);
    windowSize = window.getSize();
  } else {
    windowSize = {1280u, 720u};
    window.create(sf::VideoMode(windowSize), "PSPV2", sf::State::Windowed);
  }
  window.setFramerateLimit(60);

  // Create a view that scales from 1280x720 to actual window size
  sf::View view(sf::FloatRect({0.f, 0.f}, {1280.f, 720.f}));
  window.setView(view);

  // Load user profile
  UserProfile userProfile;
  bool profileLoaded = userProfile.load("config/user_profile.json");
  
  AppState state;
  // Only show setup if profile doesn't exist OR username is empty/default
  if (!profileLoaded || (userProfile.isFirstTimeSetup() && userProfile.getUserName().empty())) {
    state = AppState::Setup;
  } else {
    state = AppState::Intro;
  }

  // Load PSP-style UI sounds
  UiSoundBank sounds;
  sounds.load("assets/Sounds/");
  sounds.useV150Sounds(true); // Use PSP 1.50 firmware sounds (cursor, system_ok)

  SetupScreen setupScreen(sounds, userProfile);
  IntroScreen intro(sounds, "assets/images/psp_logo.png");
  GameStartupScreen gameStartup("assets/images/PSP-STARTUP.png", 
                                "assets/Sounds/Program start.mp3");
  ControllerSelectScreen controllerSelect(sounds);
  ThemeSelector themeSelector(sounds, userProfile);
  CustomThemeCreator themeCreator(sounds);
  AboutScreen aboutScreen(sounds);

  Menu menu("config/menu.json", sounds, &userProfile);
  Launcher launcher("config/settings.json");
  MenuItem pendingLaunchItem;
  ControllerSelectScreen::InputMethod selectedInputMethod;
  
  QuickMenu quickMenu(sounds);

  sf::Clock clock;
  
  // Function to check if PPSSPP is running
  auto isPPSSPPRunning = []() -> bool {
    DWORD processes[1024], needed, processCount;
    if (!EnumProcesses(processes, sizeof(processes), &needed)) {
      return false;
    }
    processCount = needed / sizeof(DWORD);
    
    for (unsigned int i = 0; i < processCount; i++) {
      if (processes[i] != 0) {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processes[i]);
        if (hProcess) {
          char processName[MAX_PATH] = {0};
          HMODULE hMod;
          DWORD cbNeeded;
          
          if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
            GetModuleBaseNameA(hProcess, hMod, processName, sizeof(processName));
            std::string name(processName);
            if (name.find("PPSSPP") != std::string::npos) {
              CloseHandle(hProcess);
              return true;
            }
          }
          CloseHandle(hProcess);
        }
      }
    }
    return false;
  };
  
  // Function to find PPSSPP window handle
  auto getPPSSPPWindow = []() -> HWND {
    return FindWindowA(nullptr, "PPSSPP");
  };
  
  // Function to pause PPSSPP by sending F12 key (default pause hotkey)
  auto pausePPSSPP = [&getPPSSPPWindow]() -> void {
    HWND ppssppWindow = getPPSSPPWindow();
    if (ppssppWindow) {
      // Send F12 key to pause (PPSSPP's default pause hotkey)
      PostMessage(ppssppWindow, WM_KEYDOWN, VK_F12, 0);
      PostMessage(ppssppWindow, WM_KEYUP, VK_F12, 0);
    }
  };
  
  // Function to unpause PPSSPP by sending F12 key again
  auto unpausePPSSPP = [&getPPSSPPWindow]() -> void {
    HWND ppssppWindow = getPPSSPPWindow();
    if (ppssppWindow) {
      // Send F12 key to unpause (toggle)
      PostMessage(ppssppWindow, WM_KEYDOWN, VK_F12, 0);
      PostMessage(ppssppWindow, WM_KEYUP, VK_F12, 0);
    }
  };
  
  // Function to terminate PPSSPP process
  auto terminatePPSSPP = []() -> void {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return;
    
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    if (Process32First(hSnapshot, &pe32)) {
      do {
        std::string processName(pe32.szExeFile);
        if (processName.find("PPSSPP") != std::string::npos) {
          HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
          if (hProcess) {
            TerminateProcess(hProcess, 0);
            CloseHandle(hProcess);
          }
        }
      } while (Process32Next(hSnapshot, &pe32));
    }
    CloseHandle(hSnapshot);
  };

  while (window.isOpen()) {
    // Check if PPSSPP is running to disable input
    bool ppssppActive = isPPSSPPRunning();
    
    while (auto event = window.pollEvent()) {
      if (event->is<sf::Event::Closed>()) {
        window.close();
      }
      
      // Always check for Touchpad button or F1 key to trigger quick menu during PPSSPP gameplay
      if (ppssppActive && !quickMenu.isVisible()) {
        // Check for Touchpad button (button 13 on PS5 controller) or F1 key
        if (const auto* buttonPressed = event->getIf<sf::Event::JoystickButtonPressed>()) {
          if (buttonPressed->button == 13) { // Touchpad button
            sounds.playSystemOk();
            pausePPSSPP(); // Pause the emulator
            window.setVisible(true);
            window.requestFocus();
            quickMenu.show();
          }
        }
        else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
          if (keyPressed->code == sf::Keyboard::Key::F1) {
            sounds.playSystemOk();
            pausePPSSPP(); // Pause the emulator
            window.setVisible(true);
            window.requestFocus();
            quickMenu.show();
          }
        }
      }
      
      // Handle quick menu input when visible
      if (quickMenu.isVisible()) {
        quickMenu.handleEvent(*event);
      }
      // Only process input if PPSSPP is not running and quick menu not visible
      else if (!ppssppActive) {
        if (state == AppState::Setup) {
          setupScreen.handleEvent(*event);
        } else if (state == AppState::Intro) {
          intro.handleEvent(*event);
        } else if (state == AppState::Menu) {
          menu.handleEvent(*event);
        } else if (state == AppState::ControllerSelect) {
          controllerSelect.handleEvent(*event);
        } else if (state == AppState::ThemeSelect) {
          themeSelector.handleEvent(*event);
        } else if (state == AppState::ThemeCreator) {
          themeCreator.handleEvent(*event);
        } else if (state == AppState::About) {
          aboutScreen.handleEvent(*event);
        }
        // No input handling during GameStartup state
      }
    }

    float dt = clock.restart().asSeconds();
    
    // Handle quick menu choice
    if (quickMenu.getChoice() == QuickMenu::Choice::ReturnToMenu) {
      terminatePPSSPP();
      quickMenu.reset();
      window.setVisible(true);
      window.requestFocus();
      state = AppState::Menu;
    }
    else if (quickMenu.getChoice() == QuickMenu::Choice::ResumeGame) {
      unpausePPSSPP(); // Unpause the emulator
      quickMenu.reset();
      window.setVisible(false);
    }
    
    // Update quick menu
    quickMenu.update(dt);

    if (state == AppState::Setup) {
      setupScreen.update(dt);
      if (setupScreen.isFinished()) {
        userProfile.save("config/user_profile.json");
        state = AppState::Intro;
      }
    } else if (state == AppState::Intro) {
      intro.update(dt);
      if (intro.isFinished()) {
        state = AppState::Menu;
      }
    } else if (state == AppState::Menu) {
      menu.update(dt);
      if (menu.wantsToLaunch()) {
        pendingLaunchItem = menu.getSelectedItem();
        
        // Check if user wants to exit
        if (pendingLaunchItem.type == "exit_app") {
          menu.stopPreviewAudio();
          sounds.playSystemOk();
          window.close();
          menu.resetLaunchRequest();
        }
        // Check if user wants factory reset
        else if (pendingLaunchItem.type == "factory_reset") {
          menu.stopPreviewAudio();
          sounds.playSystemOk();
          userProfile.factoryReset();
          userProfile.save("config/user_profile.json");
          state = AppState::Setup;
          menu.resetLaunchRequest();
        }
        // Check if user wants to change theme
        else if (pendingLaunchItem.type == "theme_select") {
          menu.stopPreviewAudio();
          sounds.playSystemOk();
          state = AppState::ThemeSelect;
          themeSelector.reset();
          menu.resetLaunchRequest();
        }
        // Check if user wants to create custom theme
        else if (pendingLaunchItem.type == "theme_creator") {
          menu.stopPreviewAudio();
          sounds.playSystemOk();
          state = AppState::ThemeCreator;
          themeCreator.reset();
          menu.resetLaunchRequest();
        }
        // Toggle time format setting
        else if (pendingLaunchItem.type == "toggle_time_format") {
          sounds.playSystemOk();
          userProfile.setUse24HourFormat(!userProfile.getUse24HourFormat());
          userProfile.save("config/user_profile.json");
          menu.resetLaunchRequest();
        }
        // Check if user wants to see About screen
        else if (pendingLaunchItem.type == "about") {
          menu.stopPreviewAudio();
          sounds.playSystemOk();
          state = AppState::About;
          aboutScreen.reset();
          menu.resetLaunchRequest();
        }
        // Only show startup screen and controller select for PSP games
        else if (pendingLaunchItem.type == "psp_iso" || pendingLaunchItem.type == "psp_eboot") {
          menu.stopPreviewAudio();
          sounds.playSystemOk(); // Play "OK" sound before controller select
          
          // Transition to ControllerSelect state
          state = AppState::ControllerSelect;
          controllerSelect.reset();
          menu.resetLaunchRequest();
        }
        // For everything else, launch directly
        else {
          menu.stopPreviewAudio();
          sounds.playSystemOk();
          launcher.launchItem(pendingLaunchItem, false);
          menu.resetLaunchRequest();
        }
      }
    } else if (state == AppState::ControllerSelect) {
      controllerSelect.update(dt);
      if (controllerSelect.isFinished()) {
        selectedInputMethod = controllerSelect.getSelectedInput();
        
        // Clear any pending events to prevent accidental button presses
        while (window.pollEvent()) {
          // Discard all events
        }
        
        // Transition to GameStartup state
        state = AppState::GameStartup;
        gameStartup.start();
      }
    } else if (state == AppState::GameStartup) {
      gameStartup.update(dt);
      if (gameStartup.isFinished()) {
        // Launch the game after startup animation with selected input method
        bool useController = (selectedInputMethod == ControllerSelectScreen::InputMethod::PS5Controller);
        
        // Minimize the window before launching the game
        window.setVisible(false);
        
        launcher.launchItem(pendingLaunchItem, useController);
        
        // Restore window visibility after game closes
        window.setVisible(true);
        window.requestFocus();
        
        state = AppState::Menu; // Return to menu
      }
    } else if (state == AppState::ThemeSelect) {
      themeSelector.update(dt);
      if (themeSelector.isFinished()) {
        if (!themeSelector.wasCancelled()) {
          // Save the selected background to profile
          std::string selectedBg = themeSelector.getSelectedBackground();
          userProfile.setTheme(selectedBg);
          userProfile.save("config/user_profile.json");
          menu.reloadBackground();
        }
        state = AppState::Menu;
      }
    } else if (state == AppState::ThemeCreator) {
      themeCreator.update(dt);
      if (themeCreator.isFinished()) {
        if (!themeCreator.wasCancelled()) {
          // TODO: Apply created theme
          auto theme = themeCreator.getCreatedTheme();
          if (theme.has_value()) {
            std::cout << "Custom theme created: " << theme->name << "\n";
          }
        }
        state = AppState::Menu;
      }
    } else if (state == AppState::About) {
      aboutScreen.update(dt);
      if (aboutScreen.isFinished()) {
        state = AppState::Menu;
      }
    }

    window.clear(sf::Color::Black);
    if (state == AppState::Setup) {
      setupScreen.draw(window);
    } else if (state == AppState::Intro) {
      intro.draw(window);
    } else if (state == AppState::Menu) {
      menu.draw(window);
    } else if (state == AppState::ControllerSelect) {
      menu.draw(window); // Draw menu in background
      controllerSelect.draw(window);
    } else if (state == AppState::GameStartup) {
      gameStartup.draw(window);
    } else if (state == AppState::ThemeSelect) {
      menu.draw(window); // Draw menu in background
      themeSelector.draw(window);
    } else if (state == AppState::ThemeCreator) {
      themeCreator.draw(window);
    } else if (state == AppState::About) {
      menu.draw(window); // Draw menu in background
      aboutScreen.draw(window);
    }
    
    // Draw quick menu on top of everything if visible
    if (quickMenu.isVisible()) {
      quickMenu.draw(window);
    }
    
    window.display();
  }

  return 0;
}
