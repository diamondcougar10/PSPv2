#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <memory>

class UiSoundBank;
class UserProfile;

class ThemeSelector {
public:
    ThemeSelector(UiSoundBank& sounds, UserProfile& profile);
    
    void reset();
    void handleEvent(const sf::Event& event);
    void update(float dt);
    void draw(sf::RenderWindow& window);
    
    bool isFinished() const { return finished_; }
    bool wasCancelled() const { return cancelled_; }
    std::string getSelectedBackground() const { return selectedBackground_; }
    
private:
    void loadBackgrounds();
    
    UiSoundBank& soundBank_;
    UserProfile& userProfile_;
    
    sf::Font font_;
    bool fontLoaded_;
    
    std::vector<std::string> backgrounds_;
    std::vector<std::unique_ptr<sf::Texture>> thumbnails_;
    
    size_t selectedIndex_;
    float scrollOffset_;
    float targetScrollOffset_;
    
    // Parallax effect
    float parallaxOffsetX_;
    float parallaxOffsetY_;
    float targetParallaxX_;
    float targetParallaxY_;
    
    std::string selectedBackground_;
    bool finished_;
    bool cancelled_;
    
    // Layout constants
    static constexpr float THUMBNAIL_WIDTH = 320.f;
    static constexpr float THUMBNAIL_HEIGHT = 180.f;
    static constexpr float THUMBNAIL_SPACING = 30.f;
    static constexpr float START_X = 100.f;
    static constexpr float START_Y = 150.f;
};
