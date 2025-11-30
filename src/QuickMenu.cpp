#include "QuickMenu.hpp"
#include <iostream>

QuickMenu::QuickMenu(UiSoundBank& sounds) 
    : sounds_(sounds), visible_(false), choice_(Choice::None), selectedOption_(0) {
    
    // Load font (SFML 3.0 API: openFromFile instead of loadFromFile)
    if (!font_.openFromFile("assets/fonts/ui_font.ttf")) {
        std::cerr << "Failed to load font for QuickMenu" << std::endl;
    }
    
    // Semi-transparent black overlay covering entire screen
    overlay_.setSize({1280.f, 720.f});
    overlay_.setFillColor(sf::Color(0, 0, 0, 180));
    overlay_.setPosition({0.f, 0.f});
    
    // Dialog box in center
    dialogBox_.setSize({600.f, 300.f});
    dialogBox_.setFillColor(sf::Color(20, 20, 30, 250));
    dialogBox_.setOutlineThickness(3.f);
    dialogBox_.setOutlineColor(sf::Color(100, 150, 255));
    dialogBox_.setPosition({340.f, 210.f}); // Center on 1280x720
    
    // Title text (SFML 3.0: font is first parameter in constructor)
    titleText_.emplace(font_, "Return to PSPV2 Menu?", 32);
    titleText_->setFillColor(sf::Color::White);
    titleText_->setPosition({420.f, 250.f});
    
    // Resume option
    resumeText_.emplace(font_, "Resume Game", 28);
    resumeText_->setFillColor(sf::Color::White);
    resumeText_->setPosition({520.f, 340.f});
    
    // Return to menu option
    returnText_.emplace(font_, "Return to Menu", 28);
    returnText_->setFillColor(sf::Color::White);
    returnText_->setPosition({510.f, 400.f});
    
    // Selection indicator (arrow or highlight)
    selectionIndicator_.setSize({450.f, 40.f});
    selectionIndicator_.setFillColor(sf::Color(100, 150, 255, 100));
    selectionIndicator_.setPosition({410.f, 335.f});
}

void QuickMenu::show() {
    visible_ = true;
    choice_ = Choice::None;
    selectedOption_ = 0;
    selectionIndicator_.setPosition({410.f, 335.f});
}

void QuickMenu::hide() {
    visible_ = false;
}

void QuickMenu::reset() {
    choice_ = Choice::None;
    selectedOption_ = 0;
    visible_ = false;
}

void QuickMenu::handleEvent(const sf::Event& event) {
    if (!visible_) return;
    
    // Handle keyboard input
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->code == sf::Keyboard::Key::Up || keyPressed->code == sf::Keyboard::Key::W) {
            sounds_.playCursor();
            selectedOption_ = (selectedOption_ == 0) ? 1 : 0;
            selectionIndicator_.setPosition({410.f, selectedOption_ == 0 ? 335.f : 395.f});
        }
        else if (keyPressed->code == sf::Keyboard::Key::Down || keyPressed->code == sf::Keyboard::Key::S) {
            sounds_.playCursor();
            selectedOption_ = (selectedOption_ == 1) ? 0 : 1;
            selectionIndicator_.setPosition({410.f, selectedOption_ == 0 ? 335.f : 395.f});
        }
        else if (keyPressed->code == sf::Keyboard::Key::Enter || keyPressed->code == sf::Keyboard::Key::Space) {
            sounds_.playSystemOk();
            choice_ = (selectedOption_ == 0) ? Choice::ResumeGame : Choice::ReturnToMenu;
            visible_ = false;
        }
        else if (keyPressed->code == sf::Keyboard::Key::Escape || keyPressed->code == sf::Keyboard::Key::F1) {
            sounds_.playCancel();
            choice_ = Choice::ResumeGame;
            visible_ = false;
        }
    }
    
    // Handle controller input
    if (const auto* buttonPressed = event.getIf<sf::Event::JoystickButtonPressed>()) {
        unsigned int button = buttonPressed->button;
        
        // D-Pad Up (usually button 11 on PS5) or Left Stick Up
        if (button == 11) {
            sounds_.playCursor();
            selectedOption_ = (selectedOption_ == 0) ? 1 : 0;
            selectionIndicator_.setPosition({410.f, selectedOption_ == 0 ? 335.f : 395.f});
        }
        // D-Pad Down (usually button 12 on PS5) or Left Stick Down
        else if (button == 12) {
            sounds_.playCursor();
            selectedOption_ = (selectedOption_ == 1) ? 0 : 1;
            selectionIndicator_.setPosition({410.f, selectedOption_ == 0 ? 335.f : 395.f});
        }
        // X button (button 0 on PS5) to confirm
        else if (button == 0) {
            sounds_.playSystemOk();
            choice_ = (selectedOption_ == 0) ? Choice::ResumeGame : Choice::ReturnToMenu;
            visible_ = false;
        }
        // Circle button (button 1 on PS5) to cancel/resume
        else if (button == 1) {
            sounds_.playCancel();
            choice_ = Choice::ResumeGame;
            visible_ = false;
        }
    }
    
    // Handle joystick axis for analog stick navigation
    if (const auto* axisMoved = event.getIf<sf::Event::JoystickMoved>()) {
        if (axisMoved->axis == sf::Joystick::Axis::Y) {
            static float lastYAxis = 0.f;
            float currentYAxis = axisMoved->position;
            
            // Up movement (negative Y axis)
            if (currentYAxis < -50.f && lastYAxis >= -50.f) {
                sounds_.playCursor();
                selectedOption_ = (selectedOption_ == 0) ? 1 : 0;
                selectionIndicator_.setPosition({410.f, selectedOption_ == 0 ? 335.f : 395.f});
            }
            // Down movement (positive Y axis)
            else if (currentYAxis > 50.f && lastYAxis <= 50.f) {
                sounds_.playCursor();
                selectedOption_ = (selectedOption_ == 1) ? 0 : 1;
                selectionIndicator_.setPosition({410.f, selectedOption_ == 0 ? 335.f : 395.f});
            }
            
            lastYAxis = currentYAxis;
        }
    }
}

void QuickMenu::update(float dt) {
    // Could add animations here if needed
}

void QuickMenu::draw(sf::RenderWindow& window) {
    if (!visible_) return;
    
    window.draw(overlay_);
    window.draw(dialogBox_);
    window.draw(selectionIndicator_);
    if (titleText_) window.draw(*titleText_);
    if (resumeText_) window.draw(*resumeText_);
    if (returnText_) window.draw(*returnText_);
}
