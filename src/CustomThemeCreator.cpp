#include "CustomThemeCreator.hpp"
#include "UiSoundBank.hpp"
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <cmath>
#include <algorithm>
#include <windows.h>

using json = nlohmann::json;

CustomThemeCreator::CustomThemeCreator(UiSoundBank& sounds)
    : soundBank_(sounds)
    , font_()
    , fontLoaded_(false)
    , currentTheme_()
    , currentMode_(EditMode::Overview)
    , currentColorTarget_(ColorTarget::Primary)
    , selectedOption_(0)
    , tempR_(0), tempG_(0), tempB_(0), tempA_(255)
    , selectedSlider_(0)
    , patternTypes_{"none", "dots", "grid", "waves", "diagonal", "checkerboard"}
    , selectedPatternIndex_(0)
    , finished_(false)
    , cancelled_(false)
    , joystickMoved_(false) {
    
    if (!font_.openFromFile("assets/fonts/ui_font.ttf")) {
        fontLoaded_ = false;
        std::cerr << "Warning: failed to load font in CustomThemeCreator\n";
    } else {
        fontLoaded_ = true;
    }
    
    // Preview texture will be created when needed
}

void CustomThemeCreator::reset() {
    currentTheme_ = CustomTheme();
    currentMode_ = EditMode::Overview;
    selectedOption_ = 0;
    finished_ = false;
    cancelled_ = false;
}

void CustomThemeCreator::handleEvent(const sf::Event& event) {
    if (finished_) return;
    
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        switch (currentMode_) {
            case EditMode::Overview: {
                if (keyPressed->code == sf::Keyboard::Key::Up) {
                    if (selectedOption_ > 0) {
                        selectedOption_--;
                        soundBank_.playCursor();
                    }
                } else if (keyPressed->code == sf::Keyboard::Key::Down) {
                    if (selectedOption_ < 6) { // 7 options total
                        selectedOption_++;
                        soundBank_.playCursor();
                    }
                } else if (keyPressed->code == sf::Keyboard::Key::Enter || keyPressed->code == sf::Keyboard::Key::Space) {
                    soundBank_.playSystemOk();
                    switch (selectedOption_) {
                        case 0: 
                            currentMode_ = EditMode::ColorPicker; 
                            currentColorTarget_ = ColorTarget::Primary;
                            selectedSlider_ = 0;
                            // Initialize temp colors from current theme
                            {
                                sf::Color& c = getCurrentColorTarget();
                                tempR_ = static_cast<int>(c.r);
                                tempG_ = static_cast<int>(c.g);
                                tempB_ = static_cast<int>(c.b);
                                tempA_ = static_cast<int>(c.a);
                            }
                            break;
                        case 1: currentMode_ = EditMode::GradientEditor; break;
                        case 2: currentMode_ = EditMode::PatternSelector; break;
                        case 3: currentMode_ = EditMode::IconCustomizer; break;
                        case 4: currentMode_ = EditMode::FontCustomizer; break;
                        case 5: currentMode_ = EditMode::Preview; break;
                        case 6: saveTheme(); finished_ = true; break;
                    }
                } else if (keyPressed->code == sf::Keyboard::Key::Escape) {
                    cancelled_ = true;
                    finished_ = true;
                    soundBank_.playCancel();
                }
                break;
            }
            
            case EditMode::ColorPicker: {
                if (keyPressed->code == sf::Keyboard::Key::Up) {
                    if (selectedSlider_ > 0) {
                        selectedSlider_--;
                        soundBank_.playCursor();
                    }
                } else if (keyPressed->code == sf::Keyboard::Key::Down) {
                    if (selectedSlider_ < 3) {
                        selectedSlider_++;
                        soundBank_.playCursor();
                    }
                } else if (keyPressed->code == sf::Keyboard::Key::Left) {
                    int* target = (selectedSlider_ == 0) ? &tempR_ : (selectedSlider_ == 1) ? &tempG_ : (selectedSlider_ == 2) ? &tempB_ : &tempA_;
                    *target = std::max(0, *target - 5);
                    updateColorFromSliders();
                } else if (keyPressed->code == sf::Keyboard::Key::Right) {
                    int* target = (selectedSlider_ == 0) ? &tempR_ : (selectedSlider_ == 1) ? &tempG_ : (selectedSlider_ == 2) ? &tempB_ : &tempA_;
                    *target = std::min(255, *target + 5);
                    updateColorFromSliders();
                } else if (keyPressed->code == sf::Keyboard::Key::Enter) {
                    soundBank_.playSystemOk();
                    currentMode_ = EditMode::Overview;
                } else if (keyPressed->code == sf::Keyboard::Key::Escape) {
                    soundBank_.playCancel();
                    currentMode_ = EditMode::Overview;
                }
                break;
            }
            
            case EditMode::GradientEditor: {
                if (keyPressed->code == sf::Keyboard::Key::Escape) {
                    soundBank_.playCancel();
                    currentMode_ = EditMode::Overview;
                }
                break;
            }
            
            case EditMode::PatternSelector: {
                if (keyPressed->code == sf::Keyboard::Key::Left) {
                    if (selectedPatternIndex_ > 0) {
                        selectedPatternIndex_--;
                        currentTheme_.pattern.type = patternTypes_[selectedPatternIndex_];
                        soundBank_.playCursor();
                    }
                } else if (keyPressed->code == sf::Keyboard::Key::Right) {
                    if (selectedPatternIndex_ < patternTypes_.size() - 1) {
                        selectedPatternIndex_++;
                        currentTheme_.pattern.type = patternTypes_[selectedPatternIndex_];
                        soundBank_.playCursor();
                    }
                } else if (keyPressed->code == sf::Keyboard::Key::Escape) {
                    soundBank_.playCancel();
                    currentMode_ = EditMode::Overview;
                }
                break;
            }
            
            case EditMode::Preview: {
                if (keyPressed->code == sf::Keyboard::Key::Escape || keyPressed->code == sf::Keyboard::Key::Enter) {
                    soundBank_.playCancel();
                    currentMode_ = EditMode::Overview;
                }
                break;
            }
            
            default:
                if (keyPressed->code == sf::Keyboard::Key::Escape) {
                    soundBank_.playCancel();
                    currentMode_ = EditMode::Overview;
                }
                break;
        }
    } else if (const auto* joystickMoved = event.getIf<sf::Event::JoystickMoved>()) {
        const float deadzone = 50.0f;
        
        if (!joystickMoved_) {
            if (joystickMoved->axis == sf::Joystick::Axis::Y || joystickMoved->axis == sf::Joystick::Axis::PovY) {
                if (joystickMoved->position > deadzone) {
                    joystickMoved_ = true;
                    // Up (inverted - positive is down on stick but should go up in menu)
                    if (currentMode_ == EditMode::Overview) {
                        if (selectedOption_ > 0) {
                            selectedOption_--;
                            soundBank_.playCursor();
                        }
                    } else if (currentMode_ == EditMode::ColorPicker) {
                        if (selectedSlider_ > 0) {
                            selectedSlider_--;
                            soundBank_.playCursor();
                        }
                    }
                } else if (joystickMoved->position < -deadzone) {
                    joystickMoved_ = true;
                    // Down (inverted - negative is up on stick but should go down in menu)
                    if (currentMode_ == EditMode::Overview) {
                        if (selectedOption_ < 6) {
                            selectedOption_++;
                            soundBank_.playCursor();
                        }
                    } else if (currentMode_ == EditMode::ColorPicker) {
                        if (selectedSlider_ < 3) {
                            selectedSlider_++;
                            soundBank_.playCursor();
                        }
                    }
                } else {
                    joystickMoved_ = false;
                }
            } else if (joystickMoved->axis == sf::Joystick::Axis::X || joystickMoved->axis == sf::Joystick::Axis::PovX) {
                if (joystickMoved->position < -deadzone) {
                    joystickMoved_ = true;
                    // Left
                    if (currentMode_ == EditMode::ColorPicker) {
                        int* target = (selectedSlider_ == 0) ? &tempR_ : (selectedSlider_ == 1) ? &tempG_ : (selectedSlider_ == 2) ? &tempB_ : &tempA_;
                        *target = std::max(0, *target - 5);
                        updateColorFromSliders();
                    } else if (currentMode_ == EditMode::PatternSelector) {
                        if (selectedPatternIndex_ > 0) {
                            selectedPatternIndex_--;
                            currentTheme_.pattern.type = patternTypes_[selectedPatternIndex_];
                            soundBank_.playCursor();
                        }
                    }
                } else if (joystickMoved->position > deadzone) {
                    joystickMoved_ = true;
                    // Right
                    if (currentMode_ == EditMode::ColorPicker) {
                        int* target = (selectedSlider_ == 0) ? &tempR_ : (selectedSlider_ == 1) ? &tempG_ : (selectedSlider_ == 2) ? &tempB_ : &tempA_;
                        *target = std::min(255, *target + 5);
                        updateColorFromSliders();
                    } else if (currentMode_ == EditMode::PatternSelector) {
                        if (selectedPatternIndex_ < patternTypes_.size() - 1) {
                            selectedPatternIndex_++;
                            currentTheme_.pattern.type = patternTypes_[selectedPatternIndex_];
                            soundBank_.playCursor();
                        }
                    }
                } else {
                    joystickMoved_ = false;
                }
            }
        } else if (std::abs(joystickMoved->position) < deadzone) {
            joystickMoved_ = false;
        }
    } else if (const auto* joystickPressed = event.getIf<sf::Event::JoystickButtonPressed>()) {
        if (joystickPressed->button == 0) { // Cross/A button
            if (currentMode_ == EditMode::Overview) {
                soundBank_.playSystemOk();
                switch (selectedOption_) {
                    case 0: 
                        currentMode_ = EditMode::ColorPicker; 
                        currentColorTarget_ = ColorTarget::Primary;
                        selectedSlider_ = 0;
                        {
                            sf::Color& c = getCurrentColorTarget();
                            tempR_ = static_cast<int>(c.r);
                            tempG_ = static_cast<int>(c.g);
                            tempB_ = static_cast<int>(c.b);
                            tempA_ = static_cast<int>(c.a);
                        }
                        break;
                    case 1: currentMode_ = EditMode::GradientEditor; break;
                    case 2: currentMode_ = EditMode::PatternSelector; break;
                    case 3: currentMode_ = EditMode::IconCustomizer; break;
                    case 4: currentMode_ = EditMode::FontCustomizer; break;
                    case 5: currentMode_ = EditMode::Preview; break;
                    case 6: saveTheme(); finished_ = true; break;
                }
            } else if (currentMode_ == EditMode::ColorPicker) {
                soundBank_.playSystemOk();
                currentMode_ = EditMode::Overview;
            } else if (currentMode_ == EditMode::Preview) {
                soundBank_.playCancel();
                currentMode_ = EditMode::Overview;
            }
        } else if (joystickPressed->button == 1) { // Circle/B button
            if (currentMode_ == EditMode::Overview) {
                cancelled_ = true;
                finished_ = true;
                soundBank_.playCancel();
            } else {
                soundBank_.playCancel();
                currentMode_ = EditMode::Overview;
            }
        }
    }
}

void CustomThemeCreator::update(float dt) {
    // Animation updates can go here
}

void CustomThemeCreator::draw(sf::RenderWindow& window) {
    if (!fontLoaded_) return;
    
    // Draw dark background
    sf::RectangleShape bg({1280.f, 720.f});
    bg.setFillColor(sf::Color(20, 20, 40));
    window.draw(bg);
    
    // Draw title
    sf::Text title(font_, "Custom Theme Creator", 36);
    title.setFillColor(sf::Color::Cyan);
    title.setStyle(sf::Text::Bold);
    sf::FloatRect titleBounds = title.getLocalBounds();
    title.setPosition({640.f - titleBounds.size.x / 2.f, 30.f});
    window.draw(title);
    
    switch (currentMode_) {
        case EditMode::Overview: drawOverview(window); break;
        case EditMode::ColorPicker: drawColorPicker(window); break;
        case EditMode::GradientEditor: drawGradientEditor(window); break;
        case EditMode::PatternSelector: drawPatternSelector(window); break;
        case EditMode::IconCustomizer: drawIconCustomizer(window); break;
        case EditMode::FontCustomizer: drawFontCustomizer(window); break;
        case EditMode::Preview: drawPreview(window); break;
    }
}

void CustomThemeCreator::drawOverview(sf::RenderWindow& window) {
    std::vector<std::string> options = {
        "1. Colors",
        "2. Gradient",
        "3. Patterns",
        "4. Icons",
        "5. Font/Text",
        "6. Preview",
        "7. Save & Exit"
    };
    
    float startY = 150.f;
    float spacing = 60.f;
    
    for (size_t i = 0; i < options.size(); ++i) {
        bool isSelected = (i == selectedOption_);
        
        sf::Text optionText(font_, options[i], isSelected ? 28 : 24);
        optionText.setFillColor(isSelected ? sf::Color::Cyan : sf::Color(180, 180, 180));
        if (isSelected) optionText.setStyle(sf::Text::Bold);
        
        sf::FloatRect bounds = optionText.getLocalBounds();
        optionText.setPosition({640.f - bounds.size.x / 2.f, startY + i * spacing});
        window.draw(optionText);
        
        if (isSelected) {
            sf::RectangleShape highlight({bounds.size.x + 40.f, bounds.size.y + 20.f});
            highlight.setPosition({640.f - bounds.size.x / 2.f - 20.f, startY + i * spacing - 5.f});
            highlight.setFillColor(sf::Color::Transparent);
            highlight.setOutlineColor(sf::Color::Cyan);
            highlight.setOutlineThickness(2.f);
            window.draw(highlight);
        }
    }
    
    // Instructions
    sf::Text instructions(font_, "Up/Down: Navigate  |  Enter: Select  |  Esc: Cancel", 18);
    instructions.setFillColor(sf::Color(150, 150, 150));
    sf::FloatRect instrBounds = instructions.getLocalBounds();
    instructions.setPosition({640.f - instrBounds.size.x / 2.f, 650.f});
    window.draw(instructions);
}

void CustomThemeCreator::drawColorPicker(sf::RenderWindow& window) {
    sf::Text subtitle(font_, "Color Editor", 28);
    subtitle.setFillColor(sf::Color::White);
    sf::FloatRect bounds = subtitle.getLocalBounds();
    subtitle.setPosition({640.f - bounds.size.x / 2.f, 100.f});
    window.draw(subtitle);
    
    // Draw color sliders
    float startY = 200.f;
    drawColorSlider(window, "Red", tempR_, 300.f, startY, sf::Color::Red);
    drawColorSlider(window, "Green", tempG_, 300.f, startY + 80.f, sf::Color::Green);
    drawColorSlider(window, "Blue", tempB_, 300.f, startY + 160.f, sf::Color::Blue);
    drawColorSlider(window, "Alpha", tempA_, 300.f, startY + 240.f, sf::Color::White);
    
    // Color preview
    sf::RectangleShape preview({200.f, 200.f});
    preview.setPosition({850.f, startY});
    preview.setFillColor(sf::Color(tempR_, tempG_, tempB_, tempA_));
    preview.setOutlineColor(sf::Color::White);
    preview.setOutlineThickness(3.f);
    window.draw(preview);
    
    // Instructions
    sf::Text instructions(font_, "Up/Down: Select Slider  |  Left/Right: Adjust  |  Enter: Apply  |  Esc: Cancel", 18);
    instructions.setFillColor(sf::Color(150, 150, 150));
    sf::FloatRect instrBounds = instructions.getLocalBounds();
    instructions.setPosition({640.f - instrBounds.size.x / 2.f, 650.f});
    window.draw(instructions);
}

void CustomThemeCreator::drawColorSlider(sf::RenderWindow& window, const std::string& label,
                                         int& value, float x, float y, sf::Color barColor) {
    bool isSelected = false;
    if (label == "Red" && selectedSlider_ == 0) isSelected = true;
    if (label == "Green" && selectedSlider_ == 1) isSelected = true;
    if (label == "Blue" && selectedSlider_ == 2) isSelected = true;
    if (label == "Alpha" && selectedSlider_ == 3) isSelected = true;
    
    sf::Text labelText(font_, label + ": " + std::to_string(value), 22);
    labelText.setFillColor(isSelected ? sf::Color::Cyan : sf::Color::White);
    labelText.setPosition({x, y});
    window.draw(labelText);
    
    // Slider bar
    sf::RectangleShape bar({400.f, 20.f});
    bar.setPosition({x, y + 35.f});
    bar.setFillColor(sf::Color(60, 60, 60));
    bar.setOutlineColor(isSelected ? sf::Color::Cyan : sf::Color(100, 100, 100));
    bar.setOutlineThickness(2.f);
    window.draw(bar);
    
    // Slider fill
    float fillWidth = (value / 255.f) * 400.f;
    sf::RectangleShape fill({fillWidth, 20.f});
    fill.setPosition({x, y + 35.f});
    fill.setFillColor(barColor);
    window.draw(fill);
}

void CustomThemeCreator::drawGradientEditor(sf::RenderWindow& window) {
    sf::Text subtitle(font_, "Gradient Editor - Coming Soon", 28);
    subtitle.setFillColor(sf::Color::White);
    sf::FloatRect bounds = subtitle.getLocalBounds();
    subtitle.setPosition({640.f - bounds.size.x / 2.f, 300.f});
    window.draw(subtitle);
}

void CustomThemeCreator::drawPatternSelector(sf::RenderWindow& window) {
    sf::Text subtitle(font_, "Pattern Selector", 28);
    subtitle.setFillColor(sf::Color::White);
    sf::FloatRect bounds = subtitle.getLocalBounds();
    subtitle.setPosition({640.f - bounds.size.x / 2.f, 100.f});
    window.draw(subtitle);
    
    // Draw pattern options
    float startX = 200.f;
    float y = 300.f;
    
    for (size_t i = 0; i < patternTypes_.size(); ++i) {
        bool isSelected = (i == selectedPatternIndex_);
        
        float x = startX + i * 150.f;
        
        // Pattern preview box
        sf::RectangleShape box({100.f, 100.f});
        box.setPosition({x, y});
        box.setFillColor(sf::Color(80, 80, 80));
        box.setOutlineColor(isSelected ? sf::Color::Cyan : sf::Color::White);
        box.setOutlineThickness(isSelected ? 3.f : 1.f);
        window.draw(box);
        
        // Pattern name
        sf::Text name(font_, patternTypes_[i], 18);
        name.setFillColor(isSelected ? sf::Color::Cyan : sf::Color::White);
        sf::FloatRect nameBounds = name.getLocalBounds();
        name.setPosition({x + 50.f - nameBounds.size.x / 2.f, y + 110.f});
        window.draw(name);
    }
    
    // Instructions
    sf::Text instructions(font_, "Left/Right: Select Pattern  |  Esc: Back", 18);
    instructions.setFillColor(sf::Color(150, 150, 150));
    sf::FloatRect instrBounds = instructions.getLocalBounds();
    instructions.setPosition({640.f - instrBounds.size.x / 2.f, 650.f});
    window.draw(instructions);
}

void CustomThemeCreator::drawIconCustomizer(sf::RenderWindow& window) {
    sf::Text subtitle(font_, "Icon Customizer - Coming Soon", 28);
    subtitle.setFillColor(sf::Color::White);
    sf::FloatRect bounds = subtitle.getLocalBounds();
    subtitle.setPosition({640.f - bounds.size.x / 2.f, 300.f});
    window.draw(subtitle);
}

void CustomThemeCreator::drawFontCustomizer(sf::RenderWindow& window) {
    sf::Text subtitle(font_, "Font Customizer - Coming Soon", 28);
    subtitle.setFillColor(sf::Color::White);
    sf::FloatRect bounds = subtitle.getLocalBounds();
    subtitle.setPosition({640.f - bounds.size.x / 2.f, 300.f});
    window.draw(subtitle);
}

void CustomThemeCreator::drawPreview(sf::RenderWindow& window) {
    // Draw preview background
    sf::RectangleShape previewBg({1280.f, 720.f});
    previewBg.setFillColor(currentTheme_.colors.primary);
    window.draw(previewBg);
    
    // Draw gradient if enabled
    if (currentTheme_.gradient.enabled) {
        // Simple vertical gradient for now
        sf::VertexArray gradient(sf::PrimitiveType::TriangleStrip, 4);
        gradient[0].position = {0.f, 0.f};
        gradient[0].color = currentTheme_.gradient.startColor;
        gradient[1].position = {1280.f, 0.f};
        gradient[1].color = currentTheme_.gradient.startColor;
        gradient[2].position = {0.f, 720.f};
        gradient[2].color = currentTheme_.gradient.endColor;
        gradient[3].position = {1280.f, 720.f};
        gradient[3].color = currentTheme_.gradient.endColor;
        window.draw(gradient);
    }
    
    // Sample UI elements with theme colors
    sf::Text sampleTitle(font_, "Sample Menu Title", 32);
    sampleTitle.setFillColor(currentTheme_.colors.textPrimary);
    sampleTitle.setPosition({100.f, 100.f});
    window.draw(sampleTitle);
    
    sf::Text sampleText(font_, "Sample menu item", 24);
    sampleText.setFillColor(currentTheme_.colors.textSecondary);
    sampleText.setPosition({100.f, 200.f});
    window.draw(sampleText);
    
    sf::RectangleShape accentBar({200.f, 50.f});
    accentBar.setPosition({100.f, 300.f});
    accentBar.setFillColor(currentTheme_.colors.accent);
    window.draw(accentBar);
    
    // Overlay text
    sf::Text overlayText(font_, "Press ESC or Enter to return", 24);
    overlayText.setFillColor(sf::Color::White);
    overlayText.setOutlineColor(sf::Color::Black);
    overlayText.setOutlineThickness(2.f);
    sf::FloatRect bounds = overlayText.getLocalBounds();
    overlayText.setPosition({640.f - bounds.size.x / 2.f, 650.f});
    window.draw(overlayText);
}

void CustomThemeCreator::applyThemeToPreview() {
    // Not needed with direct rendering approach
}

sf::Color& CustomThemeCreator::getCurrentColorTarget() {
    switch (currentColorTarget_) {
        case ColorTarget::Primary: return currentTheme_.colors.primary;
        case ColorTarget::Secondary: return currentTheme_.colors.secondary;
        case ColorTarget::Accent: return currentTheme_.colors.accent;
        case ColorTarget::TextPrimary: return currentTheme_.colors.textPrimary;
        case ColorTarget::TextSecondary: return currentTheme_.colors.textSecondary;
        case ColorTarget::IconTint: return currentTheme_.colors.iconTint;
        case ColorTarget::GradientStart: return currentTheme_.gradient.startColor;
        case ColorTarget::GradientEnd: return currentTheme_.gradient.endColor;
        case ColorTarget::PatternColor: return currentTheme_.pattern.color;
    }
    return currentTheme_.colors.primary;
}

void CustomThemeCreator::updateColorFromSliders() {
    sf::Color& color = getCurrentColorTarget();
    color.r = static_cast<unsigned char>(tempR_);
    color.g = static_cast<unsigned char>(tempG_);
    color.b = static_cast<unsigned char>(tempB_);
    color.a = static_cast<unsigned char>(tempA_);
}

void CustomThemeCreator::saveTheme() {
    // TODO: Save theme to JSON file
    std::cout << "Theme saved: " << currentTheme_.name << "\n";
}

std::optional<CustomTheme> CustomThemeCreator::getCreatedTheme() const {
    if (cancelled_) return std::nullopt;
    return currentTheme_;
}
