package com.pspv2.launcher.data

import kotlinx.serialization.SerialName
import kotlinx.serialization.Serializable

/**
 * Mirrors the original C++ MenuItem struct ([src/Menu.hpp]) but trimmed to the
 * fields that make sense on Android. Texture/sprite handles are replaced by
 * asset-relative paths that Compose/Coil resolve at draw time.
 */
@Serializable
data class MenuItem(
    val label: String = "",
    val path: String = "",
    /** psp_iso, psp_eboot, web_url, folder, pc_app, theme_select, factory_reset, about */
    val type: String = "",
    @SerialName("icon") val iconFilename: String = "",
    @SerialName("preview_image") val previewImagePath: String = "",
    @SerialName("preview_bg") val previewBgPath: String = "",
    @SerialName("cover_art") val coverArtPath: String = "",
    @SerialName("preview_audio") val previewAudioPath: String = ""
)

/** Mirrors the C++ Category struct. */
@Serializable
data class Category(
    val id: String = "",
    val label: String = "",
    @SerialName("icon") val iconFilename: String = "",
    val items: List<MenuItem> = emptyList()
)

/** Root document of config/menu.json. */
@Serializable
data class MenuConfig(
    val categories: List<Category> = emptyList()
)
