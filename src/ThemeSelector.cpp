#include "ThemeSelector.hpp"
#include "UiSoundBank.hpp"
#include "UserProfile.hpp"
#include <windows.h>
#include <iostream>
#include <algorithm>

ThemeSelector::ThemeSelector(UiSoundBank& sounds, UserProfile& profile)
    : soundBank_(sounds)
    , userProfile_(profile)
    , selectedIndex_(0)
    , scrollOffset_(0.f)
    , targetScrollOffset_(0.f)
    , parallaxOffsetX_(0.f)
    , parallaxOffsetY_(0.f)
    , targetParallaxX_(0.f)
    , targetParallaxY_(0.f)
    , finished_(false)
    , cancelled_(false) {
    
    if (!font_.openFromFile("assets/fonts/ui_font.ttf")) {
        fontLoaded_ = false;
        std::cerr << "Warning: failed to load font in ThemeSelector\n";
    } else {
        fontLoaded_ = true;
    }
    
    loadBackgrounds();
}

void ThemeSelector::reset() {
    finished_ = false;
    cancelled_ = false;
    selectedIndex_ = 0;
    scrollOffset_ = 0.f;
    targetScrollOffset_ = 0.f;
    parallaxOffsetX_ = 0.f;
    parallaxOffsetY_ = 0.f;
    targetParallaxX_ = 0.f;
    targetParallaxY_ = 0.f;
    loadBackgrounds();
}

void ThemeSelector::loadBackgrounds() {
    backgrounds_.clear();
    thumbnails_.clear();
    
    std::string searchPath = "assets/Backgrounds/*.*";
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                std::string filename = findData.cFileName;
                std::string ext = filename.substr(filename.find_last_of('.') + 1);
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                
                if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp") {
                    backgrounds_.push_back(filename);
                    
                    // Load texture directly into unique_ptr for stable memory
                    try {
                        auto tex = std::make_unique<sf::Texture>("assets/Backgrounds/" + filename);
                        thumbnails_.push_back(std::move(tex));
                        std::cout << "Loaded: " << filename << "\n";
                    } catch (const std::exception& e) {
                        std::cerr << "Failed to load background: " << filename << "\n";
                    }
                }
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }
    
    std::cout << "Loaded " << backgrounds_.size() << " background(s)\n";
}

void ThemeSelector::handleEvent(const sf::Event& event) {
    if (finished_) return;
    
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->code == sf::Keyboard::Key::Left) {
            if (selectedIndex_ > 0) {
                selectedIndex_--;
                targetParallaxX_ += 8.f;
                soundBank_.playCursor();
            }
        } else if (keyPressed->code == sf::Keyboard::Key::Right) {
            if (selectedIndex_ < backgrounds_.size() - 1) {
                selectedIndex_++;
                targetParallaxX_ -= 8.f;
                soundBank_.playCursor();
            }
        } else if (keyPressed->code == sf::Keyboard::Key::Enter || keyPressed->code == sf::Keyboard::Key::Space) {
            if (!backgrounds_.empty()) {
                selectedBackground_ = backgrounds_[selectedIndex_];
                finished_ = true;
                soundBank_.playSystemOk();
            }
        } else if (keyPressed->code == sf::Keyboard::Key::Escape || keyPressed->code == sf::Keyboard::Key::Backspace) {
            cancelled_ = true;
            finished_ = true;
            soundBank_.playCancel();
        }
    } else if (const auto* joystickMoved = event.getIf<sf::Event::JoystickMoved>()) {
        const float deadzone = 50.0f;
        
        if (joystickMoved->axis == sf::Joystick::Axis::X || joystickMoved->axis == sf::Joystick::Axis::PovX) {
            if (joystickMoved->position < -deadzone) {
                // Left
                if (selectedIndex_ > 0) {
                    selectedIndex_--;
                    targetParallaxX_ += 8.f;
                    soundBank_.playCursor();
                }
            } else if (joystickMoved->position > deadzone) {
                // Right
                if (selectedIndex_ < backgrounds_.size() - 1) {
                    selectedIndex_++;
                    targetParallaxX_ -= 8.f;
                    soundBank_.playCursor();
                }
            }
        }
    } else if (const auto* joystickPressed = event.getIf<sf::Event::JoystickButtonPressed>()) {
        if (joystickPressed->button == 0) { // Cross/A button
            if (!backgrounds_.empty()) {
                selectedBackground_ = backgrounds_[selectedIndex_];
                finished_ = true;
                soundBank_.playSystemOk();
            }
        } else if (joystickPressed->button == 1) { // Circle/B button
            cancelled_ = true;
            finished_ = true;
            soundBank_.playCancel();
        }
    }
}

void ThemeSelector::update(float dt) {
    if (backgrounds_.empty()) return;
    
    // Smooth scrolling
    const float itemWidth = THUMBNAIL_WIDTH + THUMBNAIL_SPACING;
    targetScrollOffset_ = -selectedIndex_ * itemWidth;
    
    const float lerpSpeed = 8.0f;
    scrollOffset_ += (targetScrollOffset_ - scrollOffset_) * lerpSpeed * dt;
    
    // Parallax effect with spring back
    const float parallaxLerpSpeed = 5.0f;
    const float parallaxSpringBack = 3.0f;
    
    // Smooth movement toward target
    parallaxOffsetX_ += (targetParallaxX_ - parallaxOffsetX_) * parallaxLerpSpeed * dt;
    parallaxOffsetY_ += (targetParallaxY_ - parallaxOffsetY_) * parallaxLerpSpeed * dt;
    
    // Spring back toward center
    targetParallaxX_ -= targetParallaxX_ * parallaxSpringBack * dt;
    targetParallaxY_ -= targetParallaxY_ * parallaxSpringBack * dt;
    
    // Clamp to prevent excessive movement
    parallaxOffsetX_ = std::max(-15.f, std::min(15.f, parallaxOffsetX_));
    parallaxOffsetY_ = std::max(-15.f, std::min(15.f, parallaxOffsetY_));
}

void ThemeSelector::draw(sf::RenderWindow& window) {
    // Semi-transparent background
    sf::RectangleShape overlay({1280.f, 720.f});
    overlay.setFillColor(sf::Color(0, 0, 0, 200));
    window.draw(overlay);
    
    if (!fontLoaded_) return;
    
    // Title
    sf::Text titleText(font_, "Select Background Theme", 32);
    titleText.setFillColor(sf::Color::White);
    sf::FloatRect titleBounds = titleText.getLocalBounds();
    titleText.setPosition({640.f - titleBounds.size.x / 2.f, 50.f});
    window.draw(titleText);
    
    if (backgrounds_.empty()) {
        sf::Text emptyText(font_, "No backgrounds found in assets/Backgrounds/", 24);
        emptyText.setFillColor(sf::Color(200, 200, 200));
        sf::FloatRect emptyBounds = emptyText.getLocalBounds();
        emptyText.setPosition({640.f - emptyBounds.size.x / 2.f, 300.f});
        window.draw(emptyText);
        
        sf::Text infoText(font_, "Add .png, .jpg, or .bmp files to that folder", 18);
        infoText.setFillColor(sf::Color(150, 150, 150));
        sf::FloatRect infoBounds = infoText.getLocalBounds();
        infoText.setPosition({640.f - infoBounds.size.x / 2.f, 340.f});
        window.draw(infoText);
        
        return;
    }
    
    // Draw thumbnails
    const float centerX = 640.f;
    const float itemWidth = THUMBNAIL_WIDTH + THUMBNAIL_SPACING;
    
    for (size_t i = 0; i < thumbnails_.size(); ++i) {
        float xPos = centerX + scrollOffset_ + i * itemWidth - THUMBNAIL_WIDTH / 2.f;
        float yPos = START_Y;
        
        // Only draw if visible
        if (xPos + THUMBNAIL_WIDTH < 0 || xPos > 1280.f) continue;
        
        bool isSelected = (i == selectedIndex_);
        
        // Selection highlight with parallax
        if (isSelected) {
            sf::RectangleShape highlight({THUMBNAIL_WIDTH + 10.f, THUMBNAIL_HEIGHT + 10.f});
            highlight.setPosition({xPos - 5.f + parallaxOffsetX_, yPos - 5.f + parallaxOffsetY_});
            highlight.setFillColor(sf::Color::Transparent);
            highlight.setOutlineColor(sf::Color::Cyan);
            highlight.setOutlineThickness(3.f);
            window.draw(highlight);
        }
        
        // Create sprite fresh each frame using stable texture pointer
        if (!thumbnails_[i]) continue;
        sf::Sprite thumbSprite(*thumbnails_[i]);
        
        // Thumbnail with parallax effect and scale
        float parallaxX = xPos + (isSelected ? parallaxOffsetX_ : parallaxOffsetX_ * 0.3f);
        float parallaxY = yPos + (isSelected ? parallaxOffsetY_ : parallaxOffsetY_ * 0.3f);
        
        // Scale up selected thumbnail
        float baseScaleX = THUMBNAIL_WIDTH / thumbnails_[i]->getSize().x;
        float baseScaleY = THUMBNAIL_HEIGHT / thumbnails_[i]->getSize().y;
        float baseScale = std::min(baseScaleX, baseScaleY);
        float displayScale = isSelected ? baseScale * 1.15f : baseScale;
        
        // Adjust position to keep it centered when scaling
        if (isSelected) {
            float scaleOffset = (THUMBNAIL_WIDTH * 1.15f - THUMBNAIL_WIDTH) / 2.f;
            parallaxX -= scaleOffset;
            parallaxY -= scaleOffset * (THUMBNAIL_HEIGHT / THUMBNAIL_WIDTH);
        }
        
        thumbSprite.setScale({displayScale, displayScale});
        thumbSprite.setPosition({parallaxX, parallaxY});
        
        if (!isSelected) {
            // Dim non-selected thumbnails
            sf::Color dimColor = sf::Color::White;
            dimColor.a = 150;
            thumbSprite.setColor(dimColor);
        } else {
            thumbSprite.setColor(sf::Color::White);
        }
        window.draw(thumbSprite);
        
        // Filename
        sf::Text nameText(font_, backgrounds_[i], isSelected ? 20 : 16);
        nameText.setFillColor(isSelected ? sf::Color::White : sf::Color(180, 180, 180));
        sf::FloatRect nameBounds = nameText.getLocalBounds();
        nameText.setPosition({xPos + THUMBNAIL_WIDTH / 2.f - nameBounds.size.x / 2.f, yPos + THUMBNAIL_HEIGHT + 10.f});
        window.draw(nameText);
    }
    
    // Instructions
    sf::Text instructText(font_, "Left/Right: Navigate  |  Enter/Cross: Select  |  Esc/Circle: Cancel", 18);
    instructText.setFillColor(sf::Color(180, 180, 180));
    sf::FloatRect instructBounds = instructText.getLocalBounds();
    instructText.setPosition({640.f - instructBounds.size.x / 2.f, 650.f});
    window.draw(instructText);
    
    // Current selection info
    if (!backgrounds_.empty()) {
        std::string currentInfo = "Selected: " + backgrounds_[selectedIndex_];
        sf::Text currentText(font_, currentInfo, 20);
        currentText.setFillColor(sf::Color(255, 255, 100));
        sf::FloatRect currentBounds = currentText.getLocalBounds();
        currentText.setPosition({640.f - currentBounds.size.x / 2.f, 600.f});
        window.draw(currentText);
    }
}
