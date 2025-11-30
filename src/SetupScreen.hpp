#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include "UserProfile.hpp"

class UiSoundBank;

/**
 * First-time setup screen for user profile configuration
 */
class SetupScreen {
public:
    SetupScreen(UiSoundBank& sounds, UserProfile& profile);

    void handleEvent(const sf::Event& event);
    void update(float dt);
    void draw(sf::RenderWindow& window);

    bool isFinished() const { return finished_; }

private:
    void handleTextInput(char character);
    void handleBackspace();
    
    bool finished_;
    UserProfile& profile_;
    UiSoundBank& soundBank_;
    
    std::string inputName_;
    bool nameInputActive_;
    float cursorBlinkTime_;
    
    sf::Font font_;
    bool fontLoaded_;
    
    sf::RectangleShape background_;
    sf::RectangleShape inputBox_;
};
