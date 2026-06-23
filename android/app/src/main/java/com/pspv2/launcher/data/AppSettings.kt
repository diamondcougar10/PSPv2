package com.pspv2.launcher.data

import kotlinx.serialization.Serializable

/**
 * Android equivalent of config/user_profile.json. Same field names as the C++
 * UserProfile so existing profiles remain readable.
 */
@Serializable
data class UserProfile(
    val first_time_setup: Boolean = true,
    val show_clock: Boolean = true,
    val show_date: Boolean = true,
    val theme: String = "Background3.png",
    val use_24_hour_format: Boolean = true,
    val user_name: String = ""
)

/**
 * Android equivalent of config/settings.json. The PPSSPP path fields from the
 * desktop build are intentionally dropped: on Android we launch the installed
 * PPSSPP app via Intent, so there is no executable path to configure.
 */
@Serializable
data class AppSettings(
    /** Package of the PPSSPP build to target. Defaults to the free version. */
    val ppsspp_package: String = "org.ppsspp.ppsspp",
    /** Tree URI (SAF) of the folder containing PSP ROMs, chosen by the user. */
    val games_tree_uri: String = "",
    val emulator_fullscreen: Boolean = true,
    /** Play PSP-style UI feedback sounds on navigation. */
    val ui_sounds: Boolean = true,
    /** Prefer the firmware 1.50 cursor / system-ok sound variants. */
    val use_v150_sounds: Boolean = true,
    /** Play the MP4 boot video on startup (falls back to the logo splash). */
    val boot_video: Boolean = true,
    /** Asset filename of the boot video under assets/intro/. */
    val boot_video_file: String = "CurpheyMade.mp4"
)
