#include <SFML/Graphics.hpp>
#include "Menu.hpp"
#include "Launcher.hpp"
#include "IntroScreen.hpp"
#include "GameStartupScreen.hpp"
#include "ControllerSelectScreen.hpp"
#include "SetupScreen.hpp"
#include "UserProfile.hpp"
#include "ThemeSelector.hpp"
#include "UiSoundBank.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

enum class AppState {
    Setup,
    Intro,
    Menu,
    ControllerSelect,
    GameStartup,
    ThemeSelect
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

  Menu menu("config/menu.json", sounds, &userProfile);
  Launcher launcher("config/settings.json");
  MenuItem pendingLaunchItem;
  ControllerSelectScreen::InputMethod selectedInputMethod;

  sf::Clock clock;

  while (window.isOpen()) {
    while (auto event = window.pollEvent()) {
      if (event->is<sf::Event::Closed>()) {
        window.close();
      }

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
      }
      // No input handling during GameStartup state
    }

    float dt = clock.restart().asSeconds();

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
          sounds.playSystemOk();
          window.close();
          menu.resetLaunchRequest();
        }
        // Check if user wants factory reset
        else if (pendingLaunchItem.type == "factory_reset") {
          sounds.playSystemOk();
          userProfile.factoryReset();
          userProfile.save("config/user_profile.json");
          state = AppState::Setup;
          menu.resetLaunchRequest();
        }
        // Check if user wants to change theme
        else if (pendingLaunchItem.type == "theme_select") {
          sounds.playSystemOk();
          state = AppState::ThemeSelect;
          themeSelector.reset();
          menu.resetLaunchRequest();
        }
        // Only show startup screen and controller select for PSP games
        else if (pendingLaunchItem.type == "psp_iso" || pendingLaunchItem.type == "psp_eboot") {
          sounds.playSystemOk(); // Play "OK" sound before controller select
          
          // Transition to ControllerSelect state
          state = AppState::ControllerSelect;
          controllerSelect.reset();
          menu.resetLaunchRequest();
        }
        // For everything else, launch directly
        else {
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
        launcher.launchItem(pendingLaunchItem, useController);
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
    }
    window.display();
  }

  return 0;
}
