#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <string>
#include <optional>

/**
 * PlayStation startup screen shown before launching a game
 * - Displays animated GIF/image
 * - Plays startup sound
 * - Auto-closes when animation/sound completes
 */
class GameStartupScreen {
public:
    GameStartupScreen(const std::string& imagePath, 
                      const std::string& soundPath);

    void update(float dt);
    void draw(sf::RenderWindow& window);

    bool isFinished() const { return finished_; }
    void start(); // Call this to begin the animation

private:
    bool finished_ = false;
    bool started_ = false;
    float time_ = 0.f;
    const float fadeInDuration_ = 1.0f;
    const float holdDuration_ = 2.0f;
    const float fadeOutDuration_ = 1.0f;

    sf::Texture imageTex_;
    std::optional<sf::Sprite> imageSprite_;

    sf::Music startupSound_;

    sf::RectangleShape blackBg_;
};
