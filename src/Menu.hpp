#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <string>
#include <vector>
#include <optional>
#include <memory>

class UiSoundBank;

struct MenuItem {
  std::string label;
  std::string path;
  std::string type;
  std::string iconFilename;
  sf::Texture iconTex;
  std::optional<sf::Sprite> iconSprite;

  // Preview fields
  std::string previewImagePath; // Used for preview_card
  std::string previewBgPath;
  std::string coverArtPath;     // NEW: Big cover art
  std::string previewAudioPath;

  sf::Texture previewTexture;
  std::optional<sf::Sprite> previewSprite;
  bool hasPreview = false;

  sf::Texture previewBgTexture;
  std::optional<sf::Sprite> previewBgSprite;
  bool hasPreviewBg = false;

  sf::Texture coverArtTexture;
  std::optional<sf::Sprite> coverArtSprite;
  bool hasCoverArt = false;
  
  // Audio
  std::shared_ptr<sf::SoundBuffer> previewBuffer;
  bool hasPreviewAudio = false;
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
  void stopPreviewAudio() { if (previewSoundPlayer_) previewSoundPlayer_->stop(); }

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
  size_t lastCategoryIndex_{static_cast<size_t>(-1)};
  size_t lastItemIndex_{static_cast<size_t>(-1)};
  bool launchRequested_{false};
  std::string gamesRoot_;

  // Animation
  float itemListOffset_{0.f};
  float targetItemListOffset_{0.f};
  float categoryScaleAnim_{1.0f};
  float categoryScrollOffset_{0.f};       // NEW: Current scroll position (float index)
  float targetCategoryScrollOffset_{0.f}; // NEW: Target scroll position (index)
  float bgOffsetX_{0.f};
  float bgOffsetY_{0.f};
  float targetBgOffsetX_{0.f};
  float targetBgOffsetY_{0.f};
  float previewAlpha_{0.f};

  // Rendering
  sf::Font font_;
  bool fontLoaded_{false};
  sf::Texture bgTexture_;
  std::optional<sf::Sprite> bgSprite_;
  bool bgLoaded_{false};

  // Audio
  UiSoundBank& soundBank_;
  std::optional<sf::Sound> previewSoundPlayer_;

  // User profile
  UserProfile* userProfile_;

  // Constants
  static constexpr float CATEGORY_ICON_SIZE = 80.f;          // Increased from 64
  static constexpr float CATEGORY_ICON_SIZE_SELECTED = 160.f; // Increased from 80 (Zoomed in!)
  static constexpr float CATEGORY_SPACING = 200.f;            // New spacing constant
  static constexpr float CATEGORY_Y_POS = 200.f;
  static constexpr float ITEM_LIST_START_Y = 350.f;
  static constexpr float ITEM_ROW_HEIGHT = 40.f;
};
