package com.pspv2.launcher.data

import kotlinx.serialization.Serializable

/**
 * A user-authored theme, the Android port of the CustomTheme struct in
 * [src/CustomThemeCreator.hpp]. Colours are stored as packed ARGB ints (the same
 * representation Compose's Color uses) so the JSON stays compact and portable.
 *
 * The desktop version also supported pattern overlays and per-icon tints; the
 * Android port focuses on the parts that read clearly on a TV/handheld screen: a
 * vertical background gradient plus primary/accent accent colours.
 */
@Serializable
data class CustomTheme(
    val name: String = "Custom Theme",
    /** Top colour of the background gradient (ARGB). */
    val backgroundStart: Long = 0xFF1E1E3C,
    /** Bottom colour of the background gradient (ARGB). */
    val backgroundEnd: Long = 0xFF0A0A1E,
    /** Accent colour used for selection highlights (ARGB). */
    val accent: Long = 0xFF00B4FF,
    /** Primary text colour (ARGB). */
    val textPrimary: Long = 0xFFFFFFFF
) {
    companion object {
        /** Sentinel stored in UserProfile.theme when the custom theme is active. */
        const val THEME_KEY = "__custom__"
    }
}
