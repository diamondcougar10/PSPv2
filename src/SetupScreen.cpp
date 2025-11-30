#include "SetupScreen.hpp"
#include "UiSoundBank.hpp"
#include <iostream>

SetupScreen::SetupScreen(UiSoundBank& sounds, UserProfile& profile)
    : finished_(false)
    , profile_(profile)
    , soundBank_(sounds)
    , inputName_("")
    , nameInputActive_(true)
    , cursorBlinkTime_(0.f)
    , fontLoaded_(false) {
    
    // Load font
    if (!font_.openFromFile("assets/fonts/ui_font.ttf")) {
        std::cerr << "Warning: failed to load font for setup screen\n";
    } else {
        fontLoaded_ = true;
    }
    
    // Dark blue background
    background_.setSize(sf::Vector2f(1280.f, 720.f));
    background_.setFillColor(sf::Color(20, 30, 50));
    background_.setPosition({0.f, 0.f});
    
    // Input box
    inputBox_.setSize(sf::Vector2f(500.f, 50.f));
    inputBox_.setFillColor(sf::Color(40, 50, 70));
    inputBox_.setOutlineColor(sf::Color(100, 150, 200));
    inputBox_.setOutlineThickness(2.f);
    inputBox_.setPosition({390.f, 350.f});
}

void SetupScreen::handleTextInput(char character) {
    if (character >= 32 && character < 127) { // Printable ASCII
        if (inputName_.length() < 20) {
            inputName_ += character;
            soundBank_.playCursor();
        }
    }
}

void SetupScreen::handleBackspace() {
    if (!inputName_.empty()) {
        inputName_.pop_back();
        soundBank_.playCursor();
    }
}

void SetupScreen::handleEvent(const sf::Event& event) {
    if (finished_) return;
    
    if (auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->code == sf::Keyboard::Key::Enter) {
            if (!inputName_.empty()) {
                profile_.setUserName(inputName_);
                profile_.completeSetup();
                soundBank_.playSystemOk();
                finished_ = true;
            } else {
                soundBank_.playError();
            }
        }
        else if (keyPressed->code == sf::Keyboard::Key::Backspace) {
            handleBackspace();
        }
    }
    
    if (auto* textEntered = event.getIf<sf::Event::TextEntered>()) {
        char character = static_cast<char>(textEntered->unicode);
        if (character != '\r' && character != '\n' && character != '\b') {
            handleTextInput(character);
        }
    }
}

void SetupScreen::update(float dt) {
    if (finished_) return;
    
    cursorBlinkTime_ += dt;
    if (cursorBlinkTime_ > 1.0f) {
        cursorBlinkTime_ = 0.f;
    }
}

void SetupScreen::draw(sf::RenderWindow& window) {
    window.draw(background_);
    
    if (!fontLoaded_) return;
    
    // Title
    sf::Text titleText(font_, "Welcome to PSPV2", 42);
    titleText.setFillColor(sf::Color::White);
    titleText.setPosition({640.f - titleText.getLocalBounds().size.x / 2.f, 150.f});
    window.draw(titleText);
    
    // Subtitle
    sf::Text subtitleText(font_, "First Time Setup", 28);
    subtitleText.setFillColor(sf::Color(180, 180, 180));
    subtitleText.setPosition({640.f - subtitleText.getLocalBounds().size.x / 2.f, 210.f});
    window.draw(subtitleText);
    
    // Prompt
    sf::Text promptText(font_, "Enter your name:", 24);
    promptText.setFillColor(sf::Color::White);
    promptText.setPosition({640.f - promptText.getLocalBounds().size.x / 2.f, 300.f});
    window.draw(promptText);
    
    // Input box
    window.draw(inputBox_);
    
    // Input text
    std::string displayText = inputName_;
    if (cursorBlinkTime_ < 0.5f) {
        displayText += "_";
    }
    
    sf::Text inputText(font_, displayText, 24);
    inputText.setFillColor(sf::Color::White);
    inputText.setPosition({400.f, 358.f});
    window.draw(inputText);
    
    // Instruction
    sf::Text instructionText(font_, "Press Enter to continue", 18);
    instructionText.setFillColor(sf::Color(150, 150, 150));
    instructionText.setPosition({640.f - instructionText.getLocalBounds().size.x / 2.f, 450.f});
    window.draw(instructionText);
    
    // Get current date and time
    time_t now = time(nullptr);
    tm* localTime = localtime(&now);
    char dateBuffer[64];
    char timeBuffer[64];
    strftime(dateBuffer, sizeof(dateBuffer), "%A, %B %d, %Y", localTime);
    strftime(timeBuffer, sizeof(timeBuffer), "%I:%M %p", localTime);
    
    // Display date and time
    sf::Text dateText(font_, dateBuffer, 18);
    dateText.setFillColor(sf::Color(180, 180, 180));
    dateText.setPosition({640.f - dateText.getLocalBounds().size.x / 2.f, 550.f});
    window.draw(dateText);
    
    sf::Text timeText(font_, timeBuffer, 18);
    timeText.setFillColor(sf::Color(180, 180, 180));
    timeText.setPosition({640.f - timeText.getLocalBounds().size.x / 2.f, 575.f});
    window.draw(timeText);
}
