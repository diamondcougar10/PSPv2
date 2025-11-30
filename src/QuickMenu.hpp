#pragma once
#include <SFML/Graphics.hpp>
#include <optional>
#include "UiSoundBank.hpp"

class QuickMenu {
public:
    enum class Choice {
        None,
        ReturnToMenu,
        ResumeGame
    };

    QuickMenu(UiSoundBank& sounds);
    void show();
    void hide();
    bool isVisible() const { return visible_; }
    void handleEvent(const sf::Event& event);
    void update(float dt);
    void draw(sf::RenderWindow& window);
    Choice getChoice() const { return choice_; }
    void reset();

private:
    UiSoundBank& sounds_;
    bool visible_;
    Choice choice_;
    int selectedOption_; // 0 = Resume, 1 = Return to Menu
    
    sf::RectangleShape overlay_;
    sf::RectangleShape dialogBox_;
    sf::Font font_;
    std::optional<sf::Text> titleText_;
    std::optional<sf::Text> resumeText_;
    std::optional<sf::Text> returnText_;
    sf::RectangleShape selectionIndicator_;
};
