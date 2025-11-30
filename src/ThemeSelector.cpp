#include "ThemeSelector.hpp"
#include "UiSoundBank.hpp"
#include "UserProfile.hpp"
#include <windows.h>
#include <iostream>
#include <algorithm>

ThemeSelector::ThemeSelector(UiSoundBank& sounds, UserProfile& profile)
    : soundBank_(sounds)
    , userProfile_(profile)
    , font_()
    , fontLoaded_(false)
    , themes_()
    , selectedIndex_(0)
    , scrollOffset_(0.f)
    , targetScrollOffset_(0.f)
    , cameraOffsetX_(0.f)
    , targetCameraOffsetX_(0.f)
    , selectedScale_(1.0f)
    , targetScale_(1.2f)
    , previewAlpha_(0.f)
    , targetPreviewAlpha_(100.f)
    , parallaxOffsetX_(0.f)
    , parallaxOffsetY_(0.f)
    , targetParallaxX_(0.f)
    , targetParallaxY_(0.f)
    , selectedBackground_("")
    , finished_(false)
    , cancelled_(false)
    , debugMode_(false) {  // Disable per-frame debug logging
    
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
    std::cout << "\n=== LOADING THEMES ==="<< std::endl;
    themes_.clear();
    
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
                    ThemeEntry entry;
                    entry.filename = filename;
                    entry.fullPath = "assets/Backgrounds/" + filename;
                    
                    // Create display name (remove extension)
                    size_t dotPos = filename.find_last_of('.');
                    entry.displayName = (dotPos != std::string::npos) ? filename.substr(0, dotPos) : filename;
                    
                    // Load texture ONCE - it stays in memory as part of ThemeEntry
                    if (entry.texture.loadFromFile(entry.fullPath)) {
                        std::cout << "  [" << themes_.size() << "] Loaded texture: " << entry.displayName
                                  << " from " << entry.fullPath
                                  << " | Size: " << entry.texture.getSize().x << "x" << entry.texture.getSize().y
                                  << std::endl;
                        
                        // Add to vector FIRST (don't create sprite yet - texture will move!)
                        themes_.push_back(std::move(entry));
                    } else {
                        std::cerr << "  ERROR: Failed to load texture from " << entry.fullPath << std::endl;
                    }
                }
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }
    
    std::cout << "Total themes loaded: " << themes_.size() << "\n";
    
    // NOW create sprites after all textures are in their final memory locations
    std::cout << "=== CREATING SPRITES WITH STABLE TEXTURE ADDRESSES ===" << std::endl;
    for (size_t i = 0; i < themes_.size(); ++i) {
        auto& theme = themes_[i];
        
        // Create sprite using the now-stable texture reference
        theme.thumbnail = std::make_unique<sf::Sprite>(theme.texture);
        
        // Calculate scale to fit thumbnail size
        float scaleX = THUMBNAIL_WIDTH / theme.texture.getSize().x;
        float scaleY = THUMBNAIL_HEIGHT / theme.texture.getSize().y;
        float scale = std::min(scaleX, scaleY);
        theme.thumbnail->setScale({scale, scale});
        
        std::cout << "  [" << i << "] Created sprite: " << theme.displayName 
                  << " | Texture @ " << &theme.texture
                  << " | Sprite @ " << theme.thumbnail.get()
                  << std::endl;
    }
    std::cout << std::endl;
    
    updateLayout();
}

void ThemeSelector::updateLayout() {
    // This method only updates positions, NOT textures
    // Textures are loaded once in loadBackgrounds() and persist
    if (debugMode_) {
        std::cout << "updateLayout: selectedIndex=" << selectedIndex_ 
                  << " scrollOffset=" << scrollOffset_ << std::endl;
    }
}

void ThemeSelector::handleEvent(const sf::Event& event) {
    if (finished_) return;
    
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->code == sf::Keyboard::Key::Left) {
            if (!themes_.empty()) {
                // Wrap around to end if at beginning
                if (selectedIndex_ == 0) {
                    selectedIndex_ = themes_.size() - 1;
                } else {
                    selectedIndex_--;
                }
                // Restart animations for new selection
                selectedScale_ = 1.0f;
                previewAlpha_ = 0.0f;
                targetPreviewAlpha_ = 100.0f;
                targetParallaxX_ += 8.f;
                soundBank_.playCursor();
                // Update camera to center selected item
                const float thumbWidth = THUMBNAIL_WIDTH;
                const float spacing = 30.f;
                targetCameraOffsetX_ = -selectedIndex_ * (thumbWidth + spacing);
                updateLayout();
            }
        } else if (keyPressed->code == sf::Keyboard::Key::Right) {
            if (!themes_.empty()) {
                // Wrap around to beginning if at end
                if (selectedIndex_ >= themes_.size() - 1) {
                    selectedIndex_ = 0;
                } else {
                    selectedIndex_++;
                }
                // Restart animations for new selection
                selectedScale_ = 1.0f;
                previewAlpha_ = 0.0f;
                targetPreviewAlpha_ = 100.0f;
                targetParallaxX_ -= 8.f;
                soundBank_.playCursor();
                // Update camera to center selected item
                const float thumbWidth = THUMBNAIL_WIDTH;
                const float spacing = 30.f;
                targetCameraOffsetX_ = -selectedIndex_ * (thumbWidth + spacing);
                updateLayout();
            }
        } else if (keyPressed->code == sf::Keyboard::Key::Enter || keyPressed->code == sf::Keyboard::Key::Space) {
            if (!themes_.empty()) {
                selectedBackground_ = themes_[selectedIndex_].filename;
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
                // Left - wrap around
                if (!themes_.empty()) {
                    if (selectedIndex_ == 0) {
                        selectedIndex_ = themes_.size() - 1;
                    } else {
                        selectedIndex_--;
                    }
                    // Restart animations for new selection
                    selectedScale_ = 1.0f;
                    previewAlpha_ = 0.0f;
                    targetPreviewAlpha_ = 100.0f;
                    targetParallaxX_ += 8.f;
                    soundBank_.playCursor();
                    const float thumbWidth = THUMBNAIL_WIDTH;
                    const float spacing = 30.f;
                    targetCameraOffsetX_ = -selectedIndex_ * (thumbWidth + spacing);
                    updateLayout();
                }
            } else if (joystickMoved->position > deadzone) {
                // Right - wrap around
                if (!themes_.empty()) {
                    if (selectedIndex_ >= themes_.size() - 1) {
                        selectedIndex_ = 0;
                    } else {
                        selectedIndex_++;
                    }
                    // Restart animations for new selection
                    selectedScale_ = 1.0f;
                    previewAlpha_ = 0.0f;
                    targetPreviewAlpha_ = 100.0f;
                    targetParallaxX_ -= 8.f;
                    soundBank_.playCursor();
                    const float thumbWidth = THUMBNAIL_WIDTH;
                    const float spacing = 30.f;
                    targetCameraOffsetX_ = -selectedIndex_ * (thumbWidth + spacing);
                    updateLayout();
                }
            }
        }
    } else if (const auto* joystickPressed = event.getIf<sf::Event::JoystickButtonPressed>()) {
        if (joystickPressed->button == 0) { // Cross/A button
            if (!themes_.empty()) {
                selectedBackground_ = themes_[selectedIndex_].filename;
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
    // Smooth camera follow to center selected item
    const float cameraLerpSpeed = 6.0f;
    cameraOffsetX_ += (targetCameraOffsetX_ - cameraOffsetX_) * cameraLerpSpeed * dt;
    
    // Smooth scale animation for selected thumbnail
    const float scaleLerpSpeed = 8.0f;
    selectedScale_ += (targetScale_ - selectedScale_) * scaleLerpSpeed * dt;
    
    // Smooth fade-in for background preview
    const float alphaLerpSpeed = 5.0f;
    previewAlpha_ += (targetPreviewAlpha_ - previewAlpha_) * alphaLerpSpeed * dt;
    
    // Parallax effect for selection highlight
    const float parallaxLerpSpeed = 5.0f;
    const float parallaxSpringBack = 3.0f;
    
    parallaxOffsetX_ += (targetParallaxX_ - parallaxOffsetX_) * parallaxLerpSpeed * dt;
    parallaxOffsetY_ += (targetParallaxY_ - parallaxOffsetY_) * parallaxLerpSpeed * dt;
    
    targetParallaxX_ -= targetParallaxX_ * parallaxSpringBack * dt;
    targetParallaxY_ -= targetParallaxY_ * parallaxSpringBack * dt;
    
    parallaxOffsetX_ = std::max(-15.f, std::min(15.f, parallaxOffsetX_));
    parallaxOffsetY_ = std::max(-15.f, std::min(15.f, parallaxOffsetY_));
}

void ThemeSelector::draw(sf::RenderWindow& window) {
    if (debugMode_) {
        std::cout << "\n=== THEME DRAW FRAME ===" << std::endl;
    }
    
    // Preview selected background (full screen, dimmed)
    if (!themes_.empty() && themes_[selectedIndex_].thumbnail) {
        sf::Sprite previewSprite(themes_[selectedIndex_].texture);
        
        // Scale to fill screen while maintaining aspect ratio
        sf::Vector2u texSize = themes_[selectedIndex_].texture.getSize();
        float scaleX = 1280.f / texSize.x;
        float scaleY = 720.f / texSize.y;
        float scale = std::max(scaleX, scaleY);
        previewSprite.setScale({scale, scale});
        
        // Center the preview
        float offsetX = (1280.f - texSize.x * scale) / 2.f;
        float offsetY = (720.f - texSize.y * scale) / 2.f;
        previewSprite.setPosition({offsetX, offsetY});
        
        // Apply animated alpha
        unsigned char alpha = static_cast<unsigned char>(std::clamp(previewAlpha_, 0.f, 255.f));
        previewSprite.setColor(sf::Color(255, 255, 255, alpha));
        window.draw(previewSprite);
    }
    
    // Dark overlay for better contrast
    sf::RectangleShape overlay({1280.f, 720.f});
    overlay.setFillColor(sf::Color(0, 0, 0, 150));
    window.draw(overlay);
    
    if (!fontLoaded_) return;
    
    // Title
    sf::Text titleText(font_, "Select Background Theme", 32);
    titleText.setFillColor(sf::Color::White);
    sf::FloatRect titleBounds = titleText.getLocalBounds();
    titleText.setPosition({640.f - titleBounds.size.x / 2.f, 50.f});
    window.draw(titleText);
    
    if (themes_.empty()) {
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
    
    // --- Thumbnail strip (centered on selection) ---
    const float thumbWidth  = THUMBNAIL_WIDTH;
    const float thumbHeight = THUMBNAIL_HEIGHT;
    const float spacing     = THUMBNAIL_SPACING;
    
    // Center of the screen (1280 / 2)
    const float centerX     = 640.f;
    const float yPos        = 280.f;

    for (size_t i = 0; i < themes_.size(); ++i) {
        auto& theme = themes_[i];
        if (!theme.thumbnail) continue;

        // How far is this index from the selected one?
        float indexOffset = static_cast<float>(static_cast<int>(i) - 
                                               static_cast<int>(selectedIndex_));

        // Place thumbnails relative to the center
        float xPos = centerX + indexOffset * (thumbWidth + spacing);

        bool isSelected = (i == selectedIndex_);

        // Scale for selected / non-selected
        float currentScale = isSelected ? selectedScale_ : 1.0f;
        float scaledWidth  = thumbWidth  * currentScale;
        float scaledHeight = thumbHeight * currentScale;

        float centeredX = xPos - scaledWidth  / 2.f;
        float centeredY = yPos - scaledHeight / 2.f;
        
        // Cull if completely off-screen to save draw calls
        if (centeredX + scaledWidth < -thumbWidth || 
            centeredX > 1280.f + thumbWidth) {
            continue;
        }

        // Shadow + glow for selected thumbnail
        if (isSelected) {
            sf::RectangleShape shadow({scaledWidth + 12.f, scaledHeight + 12.f});
            shadow.setPosition({centeredX - 6.f + 4.f, centeredY - 6.f + 4.f});
            shadow.setFillColor(sf::Color(0, 0, 0, 150));
            window.draw(shadow);

            sf::RectangleShape outerGlow({scaledWidth + 24.f, scaledHeight + 24.f});
            outerGlow.setPosition({centeredX - 12.f, centeredY - 12.f});
            outerGlow.setFillColor(sf::Color::Transparent);
            outerGlow.setOutlineColor(sf::Color(0, 180, 255, 80));
            outerGlow.setOutlineThickness(5.f);
            window.draw(outerGlow);

            sf::RectangleShape innerBorder({scaledWidth + 4.f, scaledHeight + 4.f});
            innerBorder.setPosition({centeredX - 2.f, centeredY - 2.f});
            innerBorder.setFillColor(sf::Color::Transparent);
            innerBorder.setOutlineColor(sf::Color::Cyan);
            innerBorder.setOutlineThickness(2.f);
            window.draw(innerBorder);
        }

        // Apply color + scale to the thumbnail sprite
        sf::Vector2f originalScale = theme.thumbnail->getScale();
        theme.thumbnail->setScale({originalScale.x * currentScale,
                                   originalScale.y * currentScale});
        theme.thumbnail->setPosition({centeredX, centeredY});

        if (isSelected)
            theme.thumbnail->setColor(sf::Color::White);
        else
            theme.thumbnail->setColor(sf::Color(200, 200, 200, 120));

        window.draw(*theme.thumbnail);

        // Restore original scale for next frame
        theme.thumbnail->setScale(originalScale);

        // Name under each thumbnail
        sf::Text nameText(font_, theme.displayName, isSelected ? 22 : 16);
        if (isSelected) {
            nameText.setFillColor(sf::Color::Cyan);
            nameText.setStyle(sf::Text::Bold);
        } else {
            nameText.setFillColor(sf::Color(160, 160, 160));
        }

        sf::FloatRect nameBounds = nameText.getLocalBounds();
        nameText.setPosition({
            xPos - nameBounds.size.x / 2.f,
            yPos + thumbHeight / 2.f + 15.f
        });
        window.draw(nameText);
    }
    
    // Instructions
    sf::Text instructText(font_, "Left/Right: Navigate  |  Enter/Cross: Select  |  Esc/Circle: Cancel", 18);
    instructText.setFillColor(sf::Color(180, 180, 180));
    sf::FloatRect instructBounds = instructText.getLocalBounds();
    instructText.setPosition({640.f - instructBounds.size.x / 2.f, 650.f});
    window.draw(instructText);
    
    // Current selection info
    if (!themes_.empty()) {
        std::string currentInfo = "Selected: " + themes_[selectedIndex_].displayName;
        sf::Text currentText(font_, currentInfo, 20);
        currentText.setFillColor(sf::Color(255, 255, 100));
        sf::FloatRect currentBounds = currentText.getLocalBounds();
        currentText.setPosition({640.f - currentBounds.size.x / 2.f, 600.f});
        window.draw(currentText);
    }
}
