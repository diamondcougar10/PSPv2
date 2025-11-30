# PSPV2 - PSP-Style UI Launcher

A PlayStation Portable (PSP) inspired UI launcher built with C++ and SFML 3.0.2. Features a custom XMB-style interface with theme support, user profiles, and PPSSPP emulator integration.

## Features

- **PSP-Style Interface**: Authentic XMB-inspired menu system with smooth animations and parallax effects
- **Theme System**: Customizable backgrounds with visual preview and selection
- **User Profiles**: Personalized settings with first-time setup wizard
- **PPSSPP Integration**: Launch PSP games directly with controller selection
- **Game Management**: Automatic ROM scanning and organization
- **Controller Support**: Full gamepad/joystick navigation with keyboard fallback
- **Settings Management**: Quick access to system settings, PPSSPP configuration, and more
- **Audio Feedback**: PSP-authentic sound effects for UI interactions

## Requirements

- Windows 10/11
- Visual Studio 2019 or later with C++17 support
- CMake 3.15+
- SFML 3.0.2 (included in repository)
- PPSSPP emulator (for game launching)

## Building

1. Clone the repository
2. Open PowerShell in the project directory
3. Run the build commands:
```powershell
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

4. The executable will be in `build/bin/Release/PSPV2.exe`

## Configuration

### Initial Setup
On first launch, you'll be prompted to:
- Enter your username
- Configure date/time display preferences
- Select your preferred theme

### Directory Structure
```
PSPV2/
├── assets/
│   ├── Backgrounds/     # Theme backgrounds (add custom .png/.jpg files)
│   ├── fonts/           # UI fonts
│   ├── Icons/           # Category and item icons
│   ├── images/          # Splash screens and UI graphics
│   ├── Sounds/          # UI sound effects
│   └── Themes/          # Default theme files
├── config/
│   ├── menu.json        # Menu structure and items
│   ├── settings.json    # Application settings
│   └── user_profile.json # User preferences (auto-generated)
├── Games/               # PSP ROM directory (.iso, .cso, .pbp)
└── Downloads/           # Auto-scanned for new ROMs
```

### Adding Games
1. Place PSP ROM files (.iso, .cso, or .pbp) in the `Games/` folder
2. Or drop them in `Downloads/` - they'll be auto-moved and added to the menu
3. ZIP archives in Downloads/ are automatically extracted

### Adding Custom Themes
1. Add background images (.png, .jpg, .bmp) to `assets/Backgrounds/`
2. Navigate to Settings → Themes in the UI
3. Select your preferred background

## Controls

### Keyboard
- Arrow Keys: Navigate menu
- Enter/Space: Select
- Escape/Backspace: Cancel/Back

### Controller (PS5/Xbox)
- D-Pad/Left Stick: Navigate
- Cross/A Button: Select
- Circle/B Button: Cancel/Back

## Menu Categories

- **Games**: PSP ROM library with UMD icons
- **Network**: Web links and online resources
- **Media**: Music, photos, video players
- **Saved Data**: Quick access to PPSSPP save files
- **Tools**: System utilities and applications
- **Settings**: 
  - PPSSPP Settings
  - Windows Settings
  - System Information
  - Device Manager
  - Themes (background customization)
  - Factory Reset
- **Exit**: Close application

## Features in Detail

### Game Startup Sequence
1. Select game from Games category
2. Choose input method (Keyboard or PS5 Controller)
3. PlayStation boot animation plays
4. Game launches in PPSSPP with selected input

### User Profile System
- Persistent user settings across sessions
- Customizable username display
- Date/time display toggles
- Theme preferences saved automatically
- Factory reset option to return to setup

### Theme System
- Visual thumbnail preview with parallax effects
- Enlarged selection indicator
- Smooth scrolling between options
- Changes apply immediately to UI background

## Technical Details

- Built with SFML 3.0.2 for cross-platform graphics
- Uses nlohmann/json for configuration parsing
- CMake-based build system
- C++17 standard
- State machine architecture for screen management
- View-based scaling (1280x720 base resolution)

## Customization

### Adding Menu Items
Edit `config/menu.json`:
```json
{
  "label": "My Application",
  "path": "C:/Path/To/App.exe",
  "type": "pc_app",
  "icon": "my-icon.png"
}
```

### Supported Item Types
- `psp_iso`: PSP game ROM
- `psp_eboot`: PSP homebrew
- `pc_app`: Windows application
- `web_url`: Web browser link
- `folder`: File explorer path
- `theme_select`: Theme selector
- `factory_reset`: Reset to defaults
- `exit_app`: Close application

## Credits

- SFML 3.0.2 - Simple and Fast Multimedia Library
- nlohmann/json - JSON for Modern C++
- PSP-inspired UI design and sound effects

## License

This project is for educational purposes. PSP and PlayStation are trademarks of Sony Interactive Entertainment.
