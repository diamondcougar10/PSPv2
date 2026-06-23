package com.pspv2.launcher.data

import android.content.Context
import kotlinx.serialization.decodeFromString
import kotlinx.serialization.encodeToString
import kotlinx.serialization.json.Json
import java.io.File

/**
 * Loads the bundled menu config from assets and reads/writes the user profile and
 * settings to the app's private storage. Replaces the desktop std::ifstream/ofstream
 * JSON handling spread across Menu.cpp, UserProfile.cpp and Launcher.cpp.
 */
class ConfigRepository(private val context: Context) {

    private val json = Json {
        ignoreUnknownKeys = true
        prettyPrint = true
        encodeDefaults = true
    }

    private val profileFile: File
        get() = File(context.filesDir, PROFILE_FILE)

    private val settingsFile: File
        get() = File(context.filesDir, SETTINGS_FILE)

    private val customThemeFile: File
        get() = File(context.filesDir, CUSTOM_THEME_FILE)

    /** Reads the XMB menu structure bundled in assets/config/menu.json. */
    fun loadMenu(): MenuConfig {
        return runCatching {
            val text = context.assets.open("config/menu.json")
                .bufferedReader()
                .use { it.readText() }
            json.decodeFromString<MenuConfig>(text)
        }.getOrElse { MenuConfig() }
    }

    /** Loads the saved profile, or a default first-time profile if none exists. */
    fun loadProfile(): UserProfile {
        if (!profileFile.exists()) return UserProfile(first_time_setup = true)
        return runCatching {
            json.decodeFromString<UserProfile>(profileFile.readText())
        }.getOrElse { UserProfile(first_time_setup = true) }
    }

    fun saveProfile(profile: UserProfile) {
        runCatching { profileFile.writeText(json.encodeToString(profile)) }
    }

    fun loadSettings(): AppSettings {
        if (!settingsFile.exists()) return AppSettings()
        return runCatching {
            json.decodeFromString<AppSettings>(settingsFile.readText())
        }.getOrElse { AppSettings() }
    }

    fun saveSettings(settings: AppSettings) {
        runCatching { settingsFile.writeText(json.encodeToString(settings)) }
    }

    /** Loads the saved custom theme, or a default one if the user never made one. */
    fun loadCustomTheme(): CustomTheme {
        if (!customThemeFile.exists()) return CustomTheme()
        return runCatching {
            json.decodeFromString<CustomTheme>(customThemeFile.readText())
        }.getOrElse { CustomTheme() }
    }

    fun saveCustomTheme(theme: CustomTheme) {
        runCatching { customThemeFile.writeText(json.encodeToString(theme)) }
    }

    /**
     * Lists ROMs that were imported via [RomImporter] into the private games folder,
     * as [MenuItem]s ready to merge into the XMB "games" category. Uses java.io.File
     * directly because this is app-private storage we always have access to.
     */
    fun scanImportedGames(): List<MenuItem> {
        val dir = RomImporter.gamesDir(context)
        val files = dir.listFiles() ?: return emptyList()
        return files
            .filter { it.isFile && it.extension.lowercase() in ROM_EXTENSIONS }
            .map {
                MenuItem(
                    label = it.nameWithoutExtension,
                    path = it.absolutePath,
                    type = "psp_iso",
                    iconFilename = "psp game.png"
                )
            }
            .sortedBy { it.label.lowercase() }
    }

    /** Factory reset: clears profile + settings, returning to the setup wizard. */
    fun factoryReset() {
        profileFile.delete()
        settingsFile.delete()
        customThemeFile.delete()
    }

    companion object {
        private const val PROFILE_FILE = "user_profile.json"
        private const val SETTINGS_FILE = "settings.json"
        private const val CUSTOM_THEME_FILE = "custom_theme.json"

        /** ROM file extensions PPSSPP can open (kept in sync with RomImporter/RomScanner). */
        private val ROM_EXTENSIONS = setOf("iso", "cso", "pbp", "chd", "prx", "elf")
    }
}
