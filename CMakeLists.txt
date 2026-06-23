cmake_minimum_required(VERSION 3.15)
project(PSPV2 VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Allow the user to configure vcpkg toolchain externally when using Visual Studio
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  # no-op; user can pass -DCMAKE_TOOLCHAIN_FILE=... if they use vcpkg
endif()

# Help CMake locate SFML when it is installed via vcpkg or a custom prefix.
if(NOT DEFINED SFML_DIR)
  if(DEFINED ENV{SFML_DIR})
    set(SFML_DIR "$ENV{SFML_DIR}")
  elseif(DEFINED ENV{VCPKG_ROOT})
    if(NOT DEFINED VCPKG_TARGET_TRIPLET)
      if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(VCPKG_TARGET_TRIPLET "x64-windows")
      else()
        set(VCPKG_TARGET_TRIPLET "x86-windows")
      endif()
    endif()
    list(APPEND CMAKE_PREFIX_PATH "$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}")
    set(SFML_DIR "$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/share/sfml")
  endif()
endif()

find_package(SFML 3 CONFIG COMPONENTS Graphics Window System Audio REQUIRED)

set(SOURCES
  src/main.cpp
  src/Menu.cpp
  src/Launcher.cpp
  src/IntroScreen.cpp
  src/GameStartupScreen.cpp
  src/ControllerSelectScreen.cpp
  src/SetupScreen.cpp
  src/UserProfile.cpp
  src/ThemeSelector.cpp
  src/UiSoundBank.cpp
  src/QuickMenu.cpp
  src/CustomThemeCreator.cpp
  src/GameMetadataExtractor.cpp
  src/RomAssetManager.cpp
  src/AboutScreen.cpp
)

add_executable(PSPV2 ${SOURCES})
target_include_directories(PSPV2 PRIVATE src external)

target_link_libraries(PSPV2 PRIVATE SFML::Graphics SFML::Window SFML::System SFML::Audio)

# Ensure runtime output goes to a sensible folder when using multi-config generators
set_target_properties(PSPV2 PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
