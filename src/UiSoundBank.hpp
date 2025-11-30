#pragma once
#include <SFML/Audio.hpp>
#include <string>

/**
 * PSP-style UI sound bank with firmware 1.00 and 1.50 sound variants.
 * Loads and manages all UI feedback sounds (cursor, decide, cancel, etc.)
 */
class UiSoundBank {
public:
    // Load all sound files from basePath (e.g., "assets/Sounds/")
    // Returns false if any file fails to load (but attempts all loads)
    bool load(const std::string& basePath);

    // Choose which firmware sounds to prefer (true = 1.50, false = 1.00)
    // Only affects sounds that have both versions (cursor, system_ok)
    void useV150Sounds(bool enable) { use150_ = enable; }

    // Play functions for each UI action:
    void playOpening();        // Boot intro sound (1.50)
    void playCursor();         // Move selection (Up/Down/Left/Right) - 1.00 or 1.50
    void playCategoryDecide(); // Change category (Left/Right on category row) - 1.00
    void playDecide();         // Confirm / launch - 1.00
    void playCancel();         // Back / escape - 1.00
    void playOption();         // Open options / context menu - 1.00
    void playSystemOk();       // "OK" / successful action - 1.00 or 1.50
    void playError();          // Error / invalid action - 1.00

private:
    bool use150_ = true; // If true, prefer 1.50 variants when available

    // Sound buffers (firmware 1.50)
    sf::SoundBuffer bufOpening150_;
    sf::SoundBuffer bufCursor150_;
    sf::SoundBuffer bufSystemOk150_;

    // Sound buffers (firmware 1.00)
    sf::SoundBuffer bufCursor100_;
    sf::SoundBuffer bufSystemOk100_;
    sf::SoundBuffer bufCancel100_;
    sf::SoundBuffer bufCategoryDecide100_;
    sf::SoundBuffer bufDecide100_;
    sf::SoundBuffer bufOption100_;
    sf::SoundBuffer bufError100_;

    // Sound instances (optional because sf::Sound has no default constructor in SFML 3)
    std::optional<sf::Sound> sndOpening_;
    std::optional<sf::Sound> sndCursor100_;
    std::optional<sf::Sound> sndCursor150_;
    std::optional<sf::Sound> sndSystemOk100_;
    std::optional<sf::Sound> sndSystemOk150_;
    std::optional<sf::Sound> sndCancel100_;
    std::optional<sf::Sound> sndCategoryDecide100_;
    std::optional<sf::Sound> sndDecide100_;
    std::optional<sf::Sound> sndOption100_;
    std::optional<sf::Sound> sndError100_;
};
