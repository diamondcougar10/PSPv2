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
    struct ThemeEntry {
        std::string filename;      // e.g., "background1.png"
        std::string displayName;   // e.g., "background1"
        std::string fullPath;      // e.g., "assets/Backgrounds/background1.png"
        sf::Texture texture;       // Persistent texture - MUST live as long as sprite
        std::unique_ptr<sf::Sprite> thumbnail;  // Persistent sprite using texture
    };
    
    void loadBackgrounds();
    void updateLayout();  // Update sprite positions based on scroll
    
    UiSoundBank& soundBank_;
    UserProfile& userProfile_;
    
    sf::Font font_;
    bool fontLoaded_;
    
    std::vector<ThemeEntry> themes_;  // All themes with persistent textures/sprites
    
    size_t selectedIndex_;
    float scrollOffset_;
    float targetScrollOffset_;
    float cameraOffsetX_;        // Smooth camera follow
    float targetCameraOffsetX_;  // Target camera position
    
    // Animation state
    float selectedScale_;        // Current scale of selected thumbnail
    float targetScale_;          // Target scale for animation
    float previewAlpha_;         // Alpha for background preview
    float targetPreviewAlpha_;   // Target alpha for preview
    
    // Parallax effect
    float parallaxOffsetX_;
    float parallaxOffsetY_;
    float targetParallaxX_;
    float targetParallaxY_;
    
    std::string selectedBackground_;
    bool finished_;
    bool cancelled_;
    bool debugMode_; // Enable debug logging
    
    // Layout constants
    static constexpr float THUMBNAIL_WIDTH = 320.f;
    static constexpr float THUMBNAIL_HEIGHT = 180.f;
    static constexpr float THUMBNAIL_SPACING = 30.f;
    static constexpr float START_X = 100.f;
    static constexpr float START_Y = 150.f;
};
