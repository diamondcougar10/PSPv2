#include "GameStartupScreen.hpp"
#include <iostream>

GameStartupScreen::GameStartupScreen(const std::string& imagePath, 
                                     const std::string& soundPath) {
    // Load startup image/GIF
    try {
        imageTex_ = sf::Texture(imagePath);
        imageTex_.setSmooth(true); // Enable smoothing
        imageSprite_.emplace(imageTex_);
        
        // Center image on screen
        sf::Vector2u imageSize = imageTex_.getSize();
        imageSprite_->setOrigin({imageSize.x / 2.f, imageSize.y / 2.f});
        imageSprite_->setPosition({640.f, 360.f}); // Center of 1280x720
        
        // Scale to fill screen
        float scaleX = 1280.f / imageSize.x;
        float scaleY = 720.f / imageSize.y;
        float scale = std::max(scaleX, scaleY); // Aspect Fill
        imageSprite_->setScale({scale, scale});

        // Start fully transparent
        imageSprite_->setColor(sf::Color(255, 255, 255, 0));
        
        std::cout << "GameStartupScreen: Loaded image: " << imagePath << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Warning: Failed to load startup image: " << imagePath 
                  << " - " << e.what() << std::endl;
    }

    // Load startup sound
    if (!startupSound_.openFromFile(soundPath)) {
        std::cerr << "Warning: Failed to load startup sound: " << soundPath << std::endl;
    } else {
        std::cout << "GameStartupScreen: Loaded sound: " << soundPath << std::endl;
    }

    // Black background
    blackBg_.setSize(sf::Vector2f(1280.f, 720.f));
    blackBg_.setFillColor(sf::Color::Black);
    blackBg_.setPosition({0.f, 0.f});
}

void GameStartupScreen::start() {
    if (!started_) {
        started_ = true;
        time_ = 0.f;
        finished_ = false;
        
        // Play the startup sound
        startupSound_.play();
        std::cout << "GameStartupScreen: Starting animation and sound" << std::endl;
    }
}

void GameStartupScreen::update(float dt) {
    if (!started_ || finished_) return;

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
        std::cout << "GameStartupScreen: Animation complete" << std::endl;
    }

    // Apply alpha to image
    if (imageSprite_) {
        imageSprite_->setColor(sf::Color(255, 255, 255, alpha));
    }
}

void GameStartupScreen::draw(sf::RenderWindow& window) {
    if (!started_) return;

    // Draw black background
    window.draw(blackBg_);

    // Draw startup image
    if (imageSprite_) {
        window.draw(*imageSprite_);
    }
}
