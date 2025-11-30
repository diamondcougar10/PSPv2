#include "UiSoundBank.hpp"
#include <iostream>

bool UiSoundBank::load(const std::string& basePath) {
    bool allSuccess = true;

    // Helper lambda to load a sound buffer
    auto loadBuffer = [&](sf::SoundBuffer& buffer, const std::string& filename) -> bool {
        try {
            buffer = sf::SoundBuffer(basePath + filename);
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to load sound: " << basePath + filename << std::endl;
            allSuccess = false;
            return false;
        }
    };

    // Load 1.50 firmware sounds
    std::cout << "Loading PSP 1.50 firmware sounds..." << std::endl;
    loadBuffer(bufOpening150_, "PSP-1.50_opening_plugin.rco-snd_opening.mp3");
    loadBuffer(bufCursor150_, "PSP-1.50_system_plugin.rco-snd_cursor.mp3");
    loadBuffer(bufSystemOk150_, "PSP-1.50_system_plugin.rco-snd_system_ok.mp3");

    // Load 1.00 firmware sounds
    std::cout << "Loading PSP 1.00 firmware sounds..." << std::endl;
    loadBuffer(bufCursor100_, "PSP-1.00_system_plugin.rco-snd_cursor.mp3");
    loadBuffer(bufSystemOk100_, "PSP-1.00_system_plugin.rco-snd_system_ok.mp3");
    loadBuffer(bufCancel100_, "PSP-1.00_system_plugin.rco-snd_cancel.mp3");
    loadBuffer(bufCategoryDecide100_, "PSP-1.00_system_plugin.rco-snd_category_decide.mp3");
    loadBuffer(bufDecide100_, "PSP-1.00_system_plugin.rco-snd_decide.mp3");
    loadBuffer(bufOption100_, "PSP-1.00_system_plugin.rco-snd_option.mp3");
    loadBuffer(bufError100_, "PSP-1.00_system_plugin.rco-snd_error.mp3");

    // Create sound instances and attach buffers
    try {
        sndOpening_.emplace(bufOpening150_);
        
        sndCursor100_.emplace(bufCursor100_);
        sndCursor150_.emplace(bufCursor150_);
        
        sndSystemOk100_.emplace(bufSystemOk100_);
        sndSystemOk150_.emplace(bufSystemOk150_);
        
        sndCancel100_.emplace(bufCancel100_);
        sndCategoryDecide100_.emplace(bufCategoryDecide100_);
        sndDecide100_.emplace(bufDecide100_);
        sndOption100_.emplace(bufOption100_);
        sndError100_.emplace(bufError100_);
    } catch (const std::exception& e) {
        std::cerr << "Error: Failed to create sound instances: " << e.what() << std::endl;
        allSuccess = false;
    }

    if (allSuccess) {
        std::cout << "All PSP UI sounds loaded successfully!" << std::endl;
    } else {
        std::cout << "Some PSP UI sounds failed to load (see warnings above)" << std::endl;
    }

    return allSuccess;
}

void UiSoundBank::playOpening() {
    if (sndOpening_) sndOpening_->play();
}

void UiSoundBank::playCursor() {
    if (use150_) {
        if (sndCursor150_) sndCursor150_->play();
    } else {
        if (sndCursor100_) sndCursor100_->play();
    }
}

void UiSoundBank::playCategoryDecide() {
    if (sndCategoryDecide100_) sndCategoryDecide100_->play();
}

void UiSoundBank::playDecide() {
    if (sndDecide100_) sndDecide100_->play();
}

void UiSoundBank::playCancel() {
    if (sndCancel100_) sndCancel100_->play();
}

void UiSoundBank::playOption() {
    if (sndOption100_) sndOption100_->play();
}

void UiSoundBank::playSystemOk() {
    if (use150_) {
        if (sndSystemOk150_) sndSystemOk150_->play();
    } else {
        if (sndSystemOk100_) sndSystemOk100_->play();
    }
}

void UiSoundBank::playError() {
    if (sndError100_) sndError100_->play();
}
