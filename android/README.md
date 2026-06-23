# PSPV2 for Android

A native Android (Kotlin + Jetpack Compose) port of the PSPV2 PSP-style XMB launcher.
It reproduces the XrossMediaBar UI as a handheld-console front-end, supports Bluetooth
gamepads (Serafim and any standard Android HID controller), and launches PSP games
through the **installed PPSSPP app** via an Android Intent.

> This `android/` project is a from-scratch rewrite. The original C++/SFML desktop
> code in the repo root is **not** compiled into the APK — Android cannot run the
> Windows-only `windows.h` / `std::system("...PPSSPP.exe")` logic. The Kotlin code
> reuses the original assets and the `menu.json` schema.

## How games run
The app does **not** embed an emulator. The user installs **PPSSPP** from the Play
Store once; PSPV2 hands each selected game to it via `Intent.ACTION_VIEW`
(`org.ppsspp.ppsspp`). This keeps the APK small and avoids PPSSPP's GPL license
infecting this codebase.

## Requirements
- Android Studio (Ladybug / 2024.2 or newer)
- Android SDK Platform 37 + Build-Tools 37 (Android 17)
- JDK 17–21 (an Adoptium JDK 21 is already installed on this machine)
- A device/emulator running Android 8.0 (API 26) or newer

On this machine the SDK is already installed at
`C:\Users\curph\AppData\Local\Android\Sdk` (platforms 36 & 37 present), and
`android/local.properties` already points to it — no extra setup needed.

## Build & run
1. Open the `android/` folder in Android Studio (`File > Open`).
2. Let Gradle sync — Studio downloads the SDK pieces and generates the Gradle
   wrapper jar automatically the first time.
3. Pick a device and press **Run**, or build an APK from the command line:
   ```powershell
   cd android
   .\gradlew.bat assembleDebug      # debug APK
   .\gradlew.bat assembleRelease    # minified release APK (needs a signing config)
   ```
   The APK lands in `app/build/outputs/apk/`.

> If `gradlew.bat` is missing, run `gradle wrapper --gradle-version 8.9` once (or just
> open the project in Android Studio, which creates the wrapper for you).

## Controls (gamepad / keyboard)
| Action  | Gamepad                    | Keyboard       |
|---------|----------------------------|----------------|
| Move    | D-pad / left stick         | Arrow keys     |
| Confirm | A / Cross                  | Enter / Space  |
| Back    | B / Circle                 | Esc / Backspace|
| Menu    | Start / Select             | Menu           |

Serafim controllers pair over Bluetooth as standard HID gamepads, so they work
through Android's normal input system with no extra setup. Pressing **Menu/Start**
on the XMB opens a quick-menu overlay (Resume / Change Theme / About / Exit).

## Project layout
```
android/app/src/main/
├── AndroidManifest.xml
├── assets/
│   ├── config/menu.json        # XMB structure (Android-adapted)
│   ├── Backgrounds/ Icons/ images/ Sounds/ fonts/ intro/   # reused from desktop
└── java/com/pspv2/launcher/
    ├── MainActivity.kt          # input loop + screen host (replaces main.cpp)
    ├── audio/UiSoundBank.kt     # SoundPool UI sfx (replaces UiSoundBank.cpp)
    ├── data/                    # models, config repo, RomScanner (SAF folder walk)
    ├── input/GamepadInput.kt    # HID/keyboard normalisation (replaces sf::Joystick)
    ├── launch/GameLauncher.kt   # PPSSPP Intent (replaces Launcher.cpp)
    ├── media/AssetCache.kt      # stages bundled MP4 for VideoView playback
    └── ui/                      # Compose XMB + screens (replaces SFML screens)
```

## Adding games
Two ways:

1. **Scan a folder (recommended).** On the XMB choose **Settings → Scan ROM Folder**,
   then pick the directory holding your `.iso` / `.cso` / `.pbp` files. PSPV2 walks it
   via the Storage Access Framework, lists every ROM under the **Games** category, and
   remembers the folder so it rescans automatically on the next launch.
2. **Edit the config.** Add items under the `games` category in
   `app/src/main/assets/config/menu.json`. For a `psp_iso` / `psp_eboot` item, `path`
   may be a `content://` URI (recommended on modern Android) or an absolute file path
   PPSSPP can read.

## Ported features
- **XMB UI** with parallax background, animated category/item selection, status clock.
- **Bluetooth gamepad + keyboard** input (Serafim and any standard HID controller).
- **PPSSPP hand-off** via Intent (free `org.ppsspp.ppsspp` or gold build).
- **PSP UI sound effects** (firmware 1.00 / 1.50 variants) via `SoundPool`.
- **Quick-menu overlay** opened with Start/Menu.
- **Custom theme creator** — RGB sliders for gradient/accent/text with a live preview.
- **ROM folder scanning** via the Storage Access Framework (persisted across reboots).
- **MP4 boot video** played full-screen on startup, with a logo-splash fallback.

Boot video and UI sounds can be toggled through `AppSettings` (`boot_video`,
`ui_sounds`, `use_v150_sounds`) stored in the app's private `settings.json`.

