#include "UserProfile.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <shlobj.h> // For SHGetKnownFolderPath

using json = nlohmann::json;

// Helper to get the AppData/Roaming/PSPV2/config path
std::filesystem::path getConfigPath() {
    PWSTR path = NULL;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &path))) {
        std::filesystem::path appDataPath(path);
        CoTaskMemFree(path);
        
        std::filesystem::path configDir = appDataPath / "PSPV2" / "config";
        std::filesystem::create_directories(configDir);
        return configDir / "user_profile.json";
    }
    // Fallback to local config if AppData fails
    return "config/user_profile.json";
}

UserProfile::UserProfile()
    : firstTimeSetup_(true)
    , userName_("")
    , theme_("default")
    , showClock_(true)
    , showDate_(true)
    , use24HourFormat_(true)
{
}

bool UserProfile::load(const std::string& profilePath) {
    // Ignore the passed profilePath and use AppData
    std::filesystem::path configPath = getConfigPath();
    std::cout << "Loading profile from: " << configPath << "\n";
    
    std::ifstream ifs(configPath);
    if (!ifs) {
        std::cout << "No profile found, starting first-time setup\n";
        return false;
    }

    try {
        json j;
        ifs >> j;
        
        firstTimeSetup_ = j.value("first_time_setup", true);
        userName_ = j.value("user_name", std::string("User"));
        theme_ = j.value("theme", std::string("default"));
        showClock_ = j.value("show_clock", true);
        showDate_ = j.value("show_date", true);
        use24HourFormat_ = j.value("use_24_hour_format", true);
        
        std::cout << "Profile loaded: Welcome back, " << userName_ << "!\n";
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading profile: " << e.what() << "\n";
        return false;
    }
}

void UserProfile::save(const std::string& profilePath) {
    // Ignore the passed profilePath and use AppData
    std::filesystem::path configPath = getConfigPath();
    std::cout << "Saving profile to: " << configPath << "\n";

    json j;
    j["first_time_setup"] = firstTimeSetup_;
    j["user_name"] = userName_;
    j["theme"] = theme_;
    j["show_clock"] = showClock_;
    j["show_date"] = showDate_;
    j["use_24_hour_format"] = use24HourFormat_;
    
    std::ofstream ofs(configPath);
    if (ofs) {
        ofs << j.dump(2); // Pretty print with 2 space indent
        std::cout << "Profile saved successfully\n";
        std::cout << "Profile saved for " << userName_ << "\n";
    } else {
        std::cerr << "Failed to save profile to " << configPath << "\n";
    }
}
