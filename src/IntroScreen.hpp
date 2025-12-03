#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <string>
#include <optional>
#include <vector>
#include <cstdio>

class UiSoundBank;

/**
 * PSP-style boot intro screen.
 * Supports playing an MP4 video via FFmpeg pipe if available.
 * Fallback to standard logo animation if video is missing.
 */
class IntroScreen {
public:
    IntroScreen(UiSoundBank& sounds,
                const std::string& logoPath);
    ~IntroScreen();

    void handleEvent(const sf::Event& event);
    void update(float dt);
    void draw(sf::RenderWindow& window);

    bool isFinished() const { return finished_; }

private:
    // Video Playback Methods
    bool tryStartVideo();
    void updateVideo(float dt);
    void stopVideo();

    bool finished_ = false;
    
    // Standard Intro State
    bool soundStarted_ = false;
    float time_ = 0.f;
    const float duration_ = 6.0f;
    const float fadeDuration_ = 0.8f;
    const float fadeInDuration_ = 1.2f;
    const float holdDuration_ = 3.0f;
    const float fadeOutDuration_ = 1.8f;

    sf::Texture logoTex_;
    std::optional<sf::Sprite> logoSprite_;
    sf::RectangleShape blackBg_;

    UiSoundBank& soundBank_;

    // Video State
    bool isVideoMode_ = false;
    FILE* ffmpegPipe_ = nullptr;
    sf::Texture videoTexture_;
    std::optional<sf::Sprite> videoSprite_;
    std::vector<uint8_t> frameBuffer_;
    sf::Music videoAudio_;
    float videoTimer_ = 0.f;
};
