#pragma once
#include <SFML/Graphics.hpp>

class UiSoundBank;

class AboutScreen {
public:
    AboutScreen(UiSoundBank& sounds);

    void handleEvent(const sf::Event& event);
    void update(float dt);
    void draw(sf::RenderWindow& window);

    bool isFinished() const { return finished_; }
    void reset() { finished_ = false; }

private:
    bool finished_;
    UiSoundBank& soundBank_;
    
    sf::Font font_;
    bool fontLoaded_;
    
    sf::RectangleShape background_;
};
