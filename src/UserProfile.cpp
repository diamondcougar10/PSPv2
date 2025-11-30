#include "UserProfile.hpp"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

UserProfile::UserProfile()
    : firstTimeSetup_(true)
    , userName_("User")
    , theme_("default")
    , showClock_(true)
    , showDate_(true) {
}

bool UserProfile::load(const std::string& profilePath) {
    std::ifstream ifs(profilePath);
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
        
        // If we have a valid username saved, mark setup as complete
        if (!userName_.empty() && userName_ != "User") {
            firstTimeSetup_ = false;
        }
        
        std::cout << "Profile loaded: Welcome back, " << userName_ << "!\n";
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading profile: " << e.what() << "\n";
        return false;
    }
}

void UserProfile::save(const std::string& profilePath) {
    json j;
    j["first_time_setup"] = firstTimeSetup_;
    j["user_name"] = userName_;
    j["theme"] = theme_;
    j["show_clock"] = showClock_;
    j["show_date"] = showDate_;
    
    std::ofstream ofs(profilePath);
    if (ofs) {
        ofs << j.dump(2); // Pretty print with 2 space indent
        std::cout << "Profile saved successfully\n";
        std::cout << "Profile saved for " << userName_ << "\n";
    } else {
        std::cerr << "Failed to save profile\n";
    }
}
