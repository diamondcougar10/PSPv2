#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <optional>

class UiSoundBank;

/**
 * PSP-style boot intro screen with:
 * - Fade-in background
 * - Logo slide-up and fade-in animation
 * - PSP opening sound from UiSoundBank
 * - Skippable with Enter/Space/Escape
 */
class IntroScreen {
public:
    IntroScreen(UiSoundBank& sounds,
                const std::string& logoPath);

    void handleEvent(const sf::Event& event);
    void update(float dt);
    void draw(sf::RenderWindow& window);

    bool isFinished() const { return finished_; }

private:
    bool finished_ = false;
    bool soundStarted_ = false;
    float time_ = 0.f;
    const float duration_ = 6.0f;        // Total intro duration
    const float fadeDuration_ = 0.8f;    // Black overlay fade duration
    const float fadeInDuration_ = 1.2f;   // Logo fade-in duration
    const float holdDuration_ = 3.0f;      // Hold at full opacity (longer)
    const float fadeOutDuration_ = 1.8f;   // Fade-out duration

    sf::Texture logoTex_;
    std::optional<sf::Sprite> logoSprite_;

    UiSoundBank& soundBank_;

    sf::RectangleShape blackBg_;
};
