#include "IntroScreen.hpp"
#include "UiSoundBank.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <filesystem>

// Windows specific for _popen
#ifdef _WIN32
    #define POPEN _popen
    #define PCLOSE _pclose
#else
    #define POPEN popen
    #define PCLOSE pclose
#endif

IntroScreen::IntroScreen(UiSoundBank& sounds,
                         const std::string& logoPath)
    : soundBank_(sounds) {
    
    // Try to start video first
    if (tryStartVideo()) {
        isVideoMode_ = true;
        std::cout << "IntroScreen: Starting video intro mode.\n";
    }

    // Always load standard logo intro assets (for fallback or sequence)
    try {
        logoTex_ = sf::Texture(logoPath);
        logoSprite_.emplace(logoTex_);
        
        // Center logo on screen
        sf::Vector2u logoSize = logoTex_.getSize();
        logoSprite_->setOrigin({logoSize.x / 2.f, logoSize.y / 2.f});
        logoSprite_->setPosition({640.f, 360.f}); // Center of 1280x720
        
        // Start at low opacity to fade in quickly
        logoSprite_->setColor(sf::Color(255, 255, 255, 50));
        
        std::cout << "IntroScreen: Loaded PSP logo: " << logoPath << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Warning: Failed to load PSP logo: " << logoPath << std::endl;
    }

    // Black background
    blackBg_.setSize(sf::Vector2f(1280.f, 720.f));
    blackBg_.setFillColor(sf::Color::Black);
    blackBg_.setPosition({0.f, 0.f});
}

IntroScreen::~IntroScreen() {
    stopVideo();
}

bool IntroScreen::tryStartVideo() {
    const std::string videoPath = "assets/intro/CurpheyMade.mp4";
    const std::string audioPath = "assets/Sounds/intro_audio.wav";

    if (!std::filesystem::exists(videoPath)) {
        std::cerr << "IntroScreen: Video not found at " << videoPath << "\n";
        return false;
    }

    // 1. Extract Audio if needed (or always to be safe)
    // We use ffmpeg to extract audio to a temp wav file
    // -y overwrites, -vn no video, -acodec pcm_s16le (wav standard)
    std::string audioCmd = "ffmpeg -i \"" + videoPath + "\" -vn -acodec pcm_s16le -ar 44100 -ac 2 \"" + audioPath + "\" -y -loglevel quiet";
    system(audioCmd.c_str());

    // 2. Load Audio
    if (videoAudio_.openFromFile(audioPath)) {
        videoAudio_.play();
    } else {
        std::cerr << "IntroScreen: Failed to load extracted audio.\n";
    }

    // 3. Open Video Pipe
    // -vf scale=1280:720,fps=30: Native 720p at 30fps
    // -f image2pipe -vcodec rawvideo -pix_fmt rgba outputs raw pixels
    std::string cmd = "ffmpeg -i \"" + videoPath + "\" -vf scale=1280:720,fps=30 -f image2pipe -vcodec rawvideo -pix_fmt rgba -loglevel quiet -";
    ffmpegPipe_ = POPEN(cmd.c_str(), "rb");

    if (!ffmpegPipe_) {
        std::cerr << "IntroScreen: Failed to open ffmpeg pipe.\n";
        return false;
    }

    // Prepare texture (1280x720)
    if (!videoTexture_.resize({1280, 720})) {
         std::cerr << "IntroScreen: Failed to create video texture.\n";
         return false;
    }
    videoTexture_.setSmooth(false);
    videoSprite_.emplace(videoTexture_);
    videoSprite_->setScale({1.0f, 1.0f}); // Native scale
    
    // Buffer for 1280x720 RGBA
    frameBuffer_.resize(1280 * 720 * 4);

    return true;
}

void IntroScreen::stopVideo() {
    if (ffmpegPipe_) {
        PCLOSE(ffmpegPipe_);
        ffmpegPipe_ = nullptr;
    }
    videoAudio_.stop();
}

void IntroScreen::handleEvent(const sf::Event& event) {
    if (auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        // Allow skipping with Enter, Space, or Escape
        if (keyPressed->code == sf::Keyboard::Key::Enter ||
            keyPressed->code == sf::Keyboard::Key::Space ||
            keyPressed->code == sf::Keyboard::Key::Escape) {
            
            if (isVideoMode_) {
                // Skip video, go to logo
                stopVideo();
                isVideoMode_ = false;
                time_ = 0.f;
            } else {
                // Skip logo, finish intro
                finished_ = true;
            }
        }
    }
}

void IntroScreen::update(float dt) {
    if (finished_) return;

    if (isVideoMode_) {
        updateVideo(dt);
        return;
    }

    // Play opening sound once
    if (!soundStarted_) {
        soundBank_.playOpening();
        soundStarted_ = true;
    }

    // Increment time
    time_ += dt;

    // Calculate animation phases
    float fadeInEnd = fadeInDuration_;
    float holdEnd = fadeInEnd + holdDuration_;
    float fadeOutEnd = holdEnd + fadeOutDuration_;

    std::uint8_t alpha = 0;

    if (time_ < fadeInEnd) {
        // Phase 1: Fade in (0 -> 255)
        float t = time_ / fadeInDuration_;
        alpha = static_cast<std::uint8_t>(255 * t);
    } else if (time_ < holdEnd) {
        // Phase 2: Hold at full opacity
        alpha = 255;
    } else if (time_ < fadeOutEnd) {
        // Phase 3: Fade out (255 -> 0)
        float t = (time_ - holdEnd) / fadeOutDuration_;
        alpha = static_cast<std::uint8_t>(255 * (1.0f - t));
    } else {
        // Animation complete
        alpha = 0;
        finished_ = true;
    }

    // Apply alpha to logo
    if (logoSprite_) {
        logoSprite_->setColor(sf::Color(255, 255, 255, alpha));
    }
}

void IntroScreen::draw(sf::RenderWindow& window) {
    if (isVideoMode_ && videoSprite_) {
        window.draw(*videoSprite_);
        return;
    }

    // Draw black background
    window.draw(blackBg_);

    // Draw fading PSP logo
    if (logoSprite_) {
        window.draw(*logoSprite_);
    }
}

void IntroScreen::updateVideo(float dt) {
    if (!ffmpegPipe_) return;

    videoTimer_ += dt;
    const float frameDuration = 1.0f / 30.0f; // Target 30 FPS

    if (videoTimer_ >= frameDuration) {
        videoTimer_ -= frameDuration;

        // Read one frame from the pipe
        size_t read = fread(frameBuffer_.data(), 1, frameBuffer_.size(), ffmpegPipe_);
        
        if (read == frameBuffer_.size()) {
            videoTexture_.update(frameBuffer_.data());
        } else {
            // End of stream or error
            std::cout << "IntroScreen: Video finished. Switching to PSP Logo.\n";
            stopVideo();
            isVideoMode_ = false;
            time_ = 0.f;
        }
    }
}
