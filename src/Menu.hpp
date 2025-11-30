#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <optional>

class UiSoundBank;

struct MenuItem {
  std::string label;
  std::string path;
  std::string type;
  std::string iconFilename;
  sf::Texture iconTex;
  std::optional<sf::Sprite> iconSprite;
};

struct Category {
  std::string id;
  std::string label;
  std::string iconFilename;
  sf::Texture iconTex;
  std::optional<sf::Sprite> iconSprite;
  std::vector<MenuItem> items;
};

class UserProfile;

class Menu {
public:
  Menu(const std::string& configPath, UiSoundBank& sounds, UserProfile* profile = nullptr);

  void handleEvent(const sf::Event& event);
  void update(float dt);
  void draw(sf::RenderWindow& window);

  bool wantsToLaunch() const { return launchRequested_; }
  MenuItem getSelectedItem() const;
  void resetLaunchRequest() { launchRequested_ = false; }
  void reloadBackground();

private:
  void loadFromFile(const std::string& configPath);
  void loadAssets();
  void loadSettings(const std::string& settingsPath);
  void scanRomsFolder();
  std::string getItemTypeDisplay(const std::string& type) const;
  sf::Vector2f getCategoryIconPosition(size_t index) const;

  std::vector<Category> categories_;
  size_t currentCategoryIndex_{0};
  size_t currentItemIndex_{0};
  bool launchRequested_{false};
  std::string gamesRoot_;

  // Animation
  float itemListOffset_{0.f};
  float targetItemListOffset_{0.f};
  float categoryScaleAnim_{1.0f};
  float bgOffsetX_{0.f};
  float bgOffsetY_{0.f};
  float targetBgOffsetX_{0.f};
  float targetBgOffsetY_{0.f};

  // Rendering
  sf::Font font_;
  bool fontLoaded_{false};
  sf::Texture bgTexture_;
  std::optional<sf::Sprite> bgSprite_;
  bool bgLoaded_{false};

  // Audio
  UiSoundBank& soundBank_;

  // User profile
  UserProfile* userProfile_;

  // Constants
  static constexpr float CATEGORY_ICON_SIZE = 64.f;
  static constexpr float CATEGORY_ICON_SIZE_SELECTED = 80.f;
  static constexpr float CATEGORY_Y_POS = 200.f;
  static constexpr float ITEM_LIST_START_Y = 350.f;
  static constexpr float ITEM_ROW_HEIGHT = 40.f;
};
