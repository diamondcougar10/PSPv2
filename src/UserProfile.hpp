#pragma once
#include <string>
#include <nlohmann/json.hpp>

class UserProfile {
public:
    UserProfile();
    
    bool load(const std::string& profilePath);
    void save(const std::string& profilePath);
    
    bool isFirstTimeSetup() const { return firstTimeSetup_; }
    void completeSetup() { firstTimeSetup_ = false; }
    
    // Getters
    std::string getUserName() const { return userName_; }
    std::string getTheme() const { return theme_; }
    bool getShowClock() const { return showClock_; }
    bool getShowDate() const { return showDate_; }
    
    // Setters
    void setUserName(const std::string& name) { userName_ = name; }
    void setTheme(const std::string& theme) { theme_ = theme; }
    void setShowClock(bool show) { showClock_ = show; }
    void setShowDate(bool show) { showDate_ = show; }
    
    // Factory reset
    void factoryReset() {
        firstTimeSetup_ = true;
        userName_ = "";
        theme_ = "default";
        showClock_ = true;
        showDate_ = true;
    }
    
private:
    bool firstTimeSetup_;
    std::string userName_;
    std::string theme_;
    bool showClock_;
    bool showDate_;
};
