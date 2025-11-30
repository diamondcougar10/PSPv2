#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

class UiSoundBank;

/**
 * Controller selection screen shown before launching a game
 * Allows user to choose between keyboard/mouse or PS5 controller
 */
class ControllerSelectScreen {
public:
    enum class InputMethod {
        KeyboardMouse,
        PS5Controller,
        None
    };

    ControllerSelectScreen(UiSoundBank& sounds);

    void handleEvent(const sf::Event& event);
    void update(float dt);
    void draw(sf::RenderWindow& window);

    bool isFinished() const { return finished_; }
    InputMethod getSelectedInput() const { return selectedInput_; }
    void reset();

private:
    bool finished_ = false;
    int selectedIndex_ = 0;
    InputMethod selectedInput_ = InputMethod::None;

    sf::Font font_;
    bool fontLoaded_ = false;

    UiSoundBank& soundBank_;

    sf::RectangleShape dimBackground_;
    sf::RectangleShape dialogBox_;

    struct Option {
        std::string label;
        InputMethod inputMethod;
        sf::Texture texture;
        std::optional<sf::Sprite> sprite;
        bool imageLoaded;
    };
    std::vector<Option> options_;
};
