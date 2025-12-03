#include "AboutScreen.hpp"
#include "UiSoundBank.hpp"
#include <iostream>

AboutScreen::AboutScreen(UiSoundBank& sounds)
    : finished_(false)
    , soundBank_(sounds)
    , fontLoaded_(false) {
    
    if (!font_.openFromFile("assets/fonts/ui_font.ttf")) {
        std::cerr << "Warning: failed to load font for about screen\n";
    } else {
        fontLoaded_ = true;
    }
    
    // Dark background overlay
    background_.setSize(sf::Vector2f(1280.f, 720.f));
    background_.setFillColor(sf::Color(0, 0, 0, 220)); // Semi-transparent black
}

void AboutScreen::handleEvent(const sf::Event& event) {
    if (finished_) return;
    
    if (auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->code == sf::Keyboard::Key::Escape || 
            keyPressed->code == sf::Keyboard::Key::Backspace ||
            keyPressed->code == sf::Keyboard::Key::Enter ||
            keyPressed->code == sf::Keyboard::Key::Space) {
            soundBank_.playCancel();
            finished_ = true;
        }
    }
    
    // Controller support: Button 1 is typically Circle (Back)
    if (auto* joyPressed = event.getIf<sf::Event::JoystickButtonPressed>()) {
        if (joyPressed->button == 1) {
            soundBank_.playCancel();
            finished_ = true;
        }
    }
}

void AboutScreen::update(float dt) {
    // Animation logic if needed
}

void AboutScreen::draw(sf::RenderWindow& window) {
    window.draw(background_);
    
    if (!fontLoaded_) return;
    
    sf::Text titleText(font_, "About PSPV2", 48);
    titleText.setFillColor(sf::Color::White);
    titleText.setPosition({640.f - titleText.getLocalBounds().size.x / 2.f, 150.f});
    window.draw(titleText);
    
    sf::Text creatorText(font_, "Created by: Ryan Curphey", 32);
    creatorText.setFillColor(sf::Color(200, 200, 200));
    creatorText.setPosition({640.f - creatorText.getLocalBounds().size.x / 2.f, 300.f});
    window.draw(creatorText);
    
    sf::Text emailText(font_, "Email: userhelppspv@gmail.com", 28);
    emailText.setFillColor(sf::Color(180, 180, 180));
    emailText.setPosition({640.f - emailText.getLocalBounds().size.x / 2.f, 350.f});
    window.draw(emailText);

    sf::Text backText(font_, "Press Back to Return", 24);
    backText.setFillColor(sf::Color(150, 150, 150));
    backText.setPosition({640.f - backText.getLocalBounds().size.x / 2.f, 600.f});
    window.draw(backText);
}
