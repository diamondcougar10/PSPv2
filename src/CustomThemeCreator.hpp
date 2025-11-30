#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <optional>

class UiSoundBank;

struct ThemeColors {
    sf::Color primary;
    sf::Color secondary;
    sf::Color accent;
    sf::Color textPrimary;
    sf::Color textSecondary;
    sf::Color iconTint;
    
    ThemeColors()
        : primary(30, 30, 60)
        , secondary(50, 50, 80)
        , accent(0, 180, 255)
        , textPrimary(255, 255, 255)
        , textSecondary(180, 180, 180)
        , iconTint(255, 255, 255) {}
};

struct GradientSettings {
    bool enabled;
    sf::Color startColor;
    sf::Color endColor;
    float angle; // 0-360 degrees
    
    GradientSettings()
        : enabled(false)
        , startColor(30, 30, 60)
        , endColor(10, 10, 30)
        , angle(90.f) {}
};

struct PatternSettings {
    std::string type; // "none", "dots", "grid", "waves", "diagonal"
    sf::Color color;
    float intensity; // 0.0 - 1.0
    float scale;
    
    PatternSettings()
        : type("none")
        , color(255, 255, 255, 30)
        , intensity(0.3f)
        , scale(1.0f) {}
};

struct CustomTheme {
    std::string name;
    ThemeColors colors;
    GradientSettings gradient;
    PatternSettings pattern;
    std::string backgroundImage; // Optional background image path
    float backgroundAlpha;
    
    CustomTheme()
        : name("Custom Theme")
        , backgroundImage("")
        , backgroundAlpha(1.0f) {}
};

class CustomThemeCreator {
public:
    CustomThemeCreator(UiSoundBank& sounds);
    
    void handleEvent(const sf::Event& event);
    void update(float dt);
    void draw(sf::RenderWindow& window);
    
    bool isFinished() const { return finished_; }
    bool wasCancelled() const { return cancelled_; }
    std::optional<CustomTheme> getCreatedTheme() const;
    
    void reset();
    
private:
    enum class EditMode {
        Overview,
        ColorPicker,
        GradientEditor,
        PatternSelector,
        IconCustomizer,
        FontCustomizer,
        Preview
    };
    
    enum class ColorTarget {
        Primary,
        Secondary,
        Accent,
        TextPrimary,
        TextSecondary,
        IconTint,
        GradientStart,
        GradientEnd,
        PatternColor
    };
    
    void drawOverview(sf::RenderWindow& window);
    void drawColorPicker(sf::RenderWindow& window);
    void drawGradientEditor(sf::RenderWindow& window);
    void drawPatternSelector(sf::RenderWindow& window);
    void drawIconCustomizer(sf::RenderWindow& window);
    void drawFontCustomizer(sf::RenderWindow& window);
    void drawPreview(sf::RenderWindow& window);
    
    void drawColorSlider(sf::RenderWindow& window, const std::string& label, 
                         int& value, float x, float y, sf::Color barColor);
    void drawToggleButton(sf::RenderWindow& window, const std::string& label,
                          bool& value, float x, float y);
    
    void applyThemeToPreview();
    void saveTheme();
    void loadBackgroundImage();
    
    sf::Color& getCurrentColorTarget();
    void updateColorFromSliders();
    
    UiSoundBank& soundBank_;
    sf::Font font_;
    bool fontLoaded_;
    
    CustomTheme currentTheme_;
    EditMode currentMode_;
    ColorTarget currentColorTarget_;
    
    // UI State
    size_t selectedOption_;
    int tempR_, tempG_, tempB_, tempA_;
    int selectedSlider_; // 0=R, 1=G, 2=B, 3=A
    
    // Preview rendering
    sf::RenderTexture previewTexture_;
    std::optional<sf::Texture> backgroundTexture_;
    std::optional<sf::Texture> patternTexture_;
    
    // Pattern options
    std::vector<std::string> patternTypes_;
    size_t selectedPatternIndex_;
    
    bool finished_;
    bool cancelled_;
    
    // Joystick state
    bool joystickMoved_;
};
