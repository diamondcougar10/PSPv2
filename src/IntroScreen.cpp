#include "IntroScreen.hpp"
#include "UiSoundBank.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

IntroScreen::IntroScreen(UiSoundBank& sounds,
                         const std::string& logoPath)
    : soundBank_(sounds) {
    // Load PSP logo
    try {
        logoTex_ = sf::Texture(logoPath);
        logoSprite_.emplace(logoTex_);
        
        // Center logo on screen
        sf::Vector2u logoSize = logoTex_.getSize();
        logoSprite_->setOrigin({logoSize.x / 2.f, logoSize.y / 2.f});
        logoSprite_->setPosition({640.f, 360.f}); // Center of 1280x720
        
        // Start at low opacity to fade in quickly
        logoSprite_->setColor(sf::Color(255, 255, 255, 50));
        
        std::cout << "IntroScreen: Loaded PSP logo: " << logoPath << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Warning: Failed to load PSP logo: " << logoPath << std::endl;
    }

    // Black background
    blackBg_.setSize(sf::Vector2f(1280.f, 720.f));
    blackBg_.setFillColor(sf::Color::Black);
    blackBg_.setPosition({0.f, 0.f});
}

void IntroScreen::handleEvent(const sf::Event& event) {
    if (auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        // Allow skipping with Enter, Space, or Escape
        if (keyPressed->code == sf::Keyboard::Key::Enter ||
            keyPressed->code == sf::Keyboard::Key::Space ||
            keyPressed->code == sf::Keyboard::Key::Escape) {
            finished_ = true;
        }
    }
}

void IntroScreen::update(float dt) {
    if (finished_) return;

    // Play opening sound once
    if (!soundStarted_) {
        soundBank_.playOpening();
        soundStarted_ = true;
    }

    // Increment time
    time_ += dt;

    // Calculate animation phases
    float fadeInEnd = fadeInDuration_;
    float holdEnd = fadeInEnd + holdDuration_;
    float fadeOutEnd = holdEnd + fadeOutDuration_;

    std::uint8_t alpha = 0;

    if (time_ < fadeInEnd) {
        // Phase 1: Fade in (0 -> 255)
        float t = time_ / fadeInDuration_;
        alpha = static_cast<std::uint8_t>(255 * t);
    } else if (time_ < holdEnd) {
        // Phase 2: Hold at full opacity
        alpha = 255;
    } else if (time_ < fadeOutEnd) {
        // Phase 3: Fade out (255 -> 0)
        float t = (time_ - holdEnd) / fadeOutDuration_;
        alpha = static_cast<std::uint8_t>(255 * (1.0f - t));
    } else {
        // Animation complete
        alpha = 0;
        finished_ = true;
    }

    // Apply alpha to logo
    if (logoSprite_) {
        logoSprite_->setColor(sf::Color(255, 255, 255, alpha));
    }
}

void IntroScreen::draw(sf::RenderWindow& window) {
    // Draw black background
    window.draw(blackBg_);

    // Draw fading PSP logo
    if (logoSprite_) {
        window.draw(*logoSprite_);
    }
}
