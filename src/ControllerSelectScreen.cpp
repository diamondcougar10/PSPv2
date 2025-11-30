#include "ControllerSelectScreen.hpp"
#include "UiSoundBank.hpp"
#include <iostream>

ControllerSelectScreen::ControllerSelectScreen(UiSoundBank& sounds)
    : soundBank_(sounds) {
    
    // Load font
    if (!font_.openFromFile("assets/fonts/ui_font.ttf")) {
        fontLoaded_ = false;
        std::cerr << "Warning: failed to load font for controller select\n";
    } else {
        fontLoaded_ = true;
    }

    // Setup options with images
    options_.resize(2);
    
    // Keyboard option
    options_[0].label = "Keyboard & Mouse";
    options_[0].inputMethod = InputMethod::KeyboardMouse;
    try {
        options_[0].texture = sf::Texture("assets/images/KeyBoard-Option.png");
        options_[0].sprite.emplace(options_[0].texture);
        options_[0].imageLoaded = true;
        std::cout << "Loaded keyboard option image\n";
    } catch (const std::exception& e) {
        std::cerr << "Failed to load keyboard image: " << e.what() << "\n";
        options_[0].imageLoaded = false;
    }
    
    // Controller option
    options_[1].label = "PS5 Controller";
    options_[1].inputMethod = InputMethod::PS5Controller;
    try {
        options_[1].texture = sf::Texture("assets/images/Controller-Option.png");
        options_[1].sprite.emplace(options_[1].texture);
        options_[1].imageLoaded = true;
        std::cout << "Loaded controller option image\n";
    } catch (const std::exception& e) {
        std::cerr << "Failed to load controller image: " << e.what() << "\n";
        options_[1].imageLoaded = false;
    }

    // Dim background
    dimBackground_.setSize(sf::Vector2f(1280.f, 720.f));
    dimBackground_.setFillColor(sf::Color(0, 0, 0, 180));
    dimBackground_.setPosition({0.f, 0.f});

    // Dialog box
    dialogBox_.setSize(sf::Vector2f(900.f, 450.f));
    dialogBox_.setFillColor(sf::Color(40, 40, 40, 240));
    dialogBox_.setOutlineColor(sf::Color(100, 100, 100));
    dialogBox_.setOutlineThickness(2.f);
    dialogBox_.setPosition({190.f, 135.f}); // Center on 1280x720
}

void ControllerSelectScreen::reset() {
    finished_ = false;
    selectedIndex_ = 0;
    selectedInput_ = InputMethod::None;
}

void ControllerSelectScreen::handleEvent(const sf::Event& event) {
    if (finished_) return;

    // Handle keyboard input
    if (auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->code == sf::Keyboard::Key::Left || keyPressed->code == sf::Keyboard::Key::Up) {
            soundBank_.playCursor();
            selectedIndex_ = (selectedIndex_ - 1 + options_.size()) % options_.size();
        }
        else if (keyPressed->code == sf::Keyboard::Key::Right || keyPressed->code == sf::Keyboard::Key::Down) {
            soundBank_.playCursor();
            selectedIndex_ = (selectedIndex_ + 1) % options_.size();
        }
        else if (keyPressed->code == sf::Keyboard::Key::Enter || 
                 keyPressed->code == sf::Keyboard::Key::Space) {
            soundBank_.playSystemOk();
            selectedInput_ = options_[selectedIndex_].inputMethod;
            finished_ = true;
        }
        else if (keyPressed->code == sf::Keyboard::Key::Escape) {
            soundBank_.playCancel();
            selectedInput_ = InputMethod::KeyboardMouse; // Default
            finished_ = true;
        }
    }
    
    // Handle gamepad/joystick button presses
    if (auto* joystickButtonPressed = event.getIf<sf::Event::JoystickButtonPressed>()) {
        // Button 0 = Cross/A (confirm)
        if (joystickButtonPressed->button == 0) {
            soundBank_.playSystemOk();
            selectedInput_ = options_[selectedIndex_].inputMethod;
            finished_ = true;
        }
        // Button 1 = Circle/B (back/default)
        else if (joystickButtonPressed->button == 1) {
            soundBank_.playCancel();
            selectedInput_ = InputMethod::KeyboardMouse;
            finished_ = true;
        }
    }
    
    // Handle D-pad/stick movement
    if (auto* joystickMoved = event.getIf<sf::Event::JoystickMoved>()) {
        const float deadzone = 50.f;
        
        // Horizontal axis for selection
        if (joystickMoved->axis == sf::Joystick::Axis::X || joystickMoved->axis == sf::Joystick::Axis::PovX) {
            if (joystickMoved->position < -deadzone) {
                soundBank_.playCursor();
                selectedIndex_ = (selectedIndex_ - 1 + options_.size()) % options_.size();
            } else if (joystickMoved->position > deadzone) {
                soundBank_.playCursor();
                selectedIndex_ = (selectedIndex_ + 1) % options_.size();
            }
        }
    }
}

void ControllerSelectScreen::update(float dt) {
    // Nothing to update for now
}

void ControllerSelectScreen::draw(sf::RenderWindow& window) {
    // Draw dim background
    window.draw(dimBackground_);

    // Draw dialog box
    window.draw(dialogBox_);

    if (!fontLoaded_) return;

    // Draw title
    sf::Text titleText(font_, "Select Input Method", 28);
    titleText.setFillColor(sf::Color::White);
    titleText.setPosition({640.f - titleText.getLocalBounds().size.x / 2.f, 170.f});
    window.draw(titleText);

    // Draw option images horizontally
    float centerX = 640.f;
    float centerY = 340.f;
    float buttonSpacing = 250.f;

    for (size_t i = 0; i < options_.size(); ++i) {
        bool isSelected = (i == static_cast<size_t>(selectedIndex_));
        
        if (options_[i].imageLoaded && options_[i].sprite) {
            // Position button images horizontally
            float xPos = centerX - buttonSpacing + (i * buttonSpacing * 2);
            
            // Center the sprite
            sf::Vector2u imageSize = options_[i].texture.getSize();
            options_[i].sprite->setOrigin({imageSize.x / 2.f, imageSize.y / 2.f});
            options_[i].sprite->setPosition({xPos, centerY});
            
            // Highlight selected option
            if (isSelected) {
                options_[i].sprite->setColor(sf::Color::White);
                
                // Draw selection box around selected option
                sf::RectangleShape selectionBox;
                selectionBox.setSize(sf::Vector2f(imageSize.x + 20.f, imageSize.y + 20.f));
                selectionBox.setFillColor(sf::Color::Transparent);
                selectionBox.setOutlineColor(sf::Color(255, 200, 0));
                selectionBox.setOutlineThickness(4.f);
                selectionBox.setOrigin({(imageSize.x + 20.f) / 2.f, (imageSize.y + 20.f) / 2.f});
                selectionBox.setPosition({xPos, centerY});
                window.draw(selectionBox);
            } else {
                options_[i].sprite->setColor(sf::Color(180, 180, 180));
            }
            
            window.draw(*options_[i].sprite);
        } else {
            // Fallback to text if image didn't load
            sf::Text optionText(font_, options_[i].label, 20);
            float xPos = centerX - buttonSpacing + (i * buttonSpacing * 2);
            
            if (isSelected) {
                optionText.setFillColor(sf::Color(255, 200, 0));
                optionText.setStyle(sf::Text::Bold);
            } else {
                optionText.setFillColor(sf::Color(180, 180, 180));
            }
            
            optionText.setPosition({xPos - optionText.getLocalBounds().size.x / 2.f, centerY});
            window.draw(optionText);
        }
    }

    // Draw instruction text
    sf::Text instructionText(font_, "Use Arrow Keys to select, Press Enter to confirm", 18);
    instructionText.setFillColor(sf::Color(150, 150, 150));
    instructionText.setPosition({640.f - instructionText.getLocalBounds().size.x / 2.f, 520.f});
    window.draw(instructionText);
}
