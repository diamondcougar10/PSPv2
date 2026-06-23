package com.pspv2.launcher.data

import android.content.Context
import android.util.Log
import java.io.File

/**
 * Extracts and caches a PSP ROM's presentation assets (icon, background, title and
 * intro audio), the Android counterpart of the desktop `RomAssetManager`.
 *
 * Each game gets a folder under `filesDir/previews/<gameId>/` containing ICON0.PNG,
 * PIC1.PNG, SND0.AT3 and title.txt. On a cache hit the existing files are reused so
 * the (relatively expensive) ROM parse only happens once per game.
 */
object GameAssetCache {

    private const val TAG = "GameAssetCache"

    /** Paths to a game's cached assets; blank when that asset wasn't present. */
    data class CachedAssets(
        val title: String = "",
        val iconPath: String = "",
        val backgroundPath: String = "",
        val audioPath: String = ""
    )

    /**
     * Returns the cached assets for [source] (an absolute ROM path or content:// URI),
     * extracting and caching them on first use. [label] seeds the cache folder name so
     * different ROMs don't collide.
     */
    fun getOrExtract(context: Context, source: String, label: String): CachedAssets {
        val gameId = sanitize(label).ifBlank { "game_${source.hashCode()}" }
        val dir = File(File(context.filesDir, "previews"), gameId)

        val iconFile = File(dir, "ICON0.PNG")
        val bgFile = File(dir, "PIC1.PNG")
        val sndFile = File(dir, "SND0.AT3")
        val titleFile = File(dir, "title.txt")

        // Cache hit: at least one visual asset already extracted.
        if (iconFile.exists() || bgFile.exists()) {
            return CachedAssets(
                title = if (titleFile.exists()) runCatching { titleFile.readText().trim() }.getOrDefault("") else "",
                iconPath = if (iconFile.exists()) iconFile.absolutePath else "",
                backgroundPath = if (bgFile.exists()) bgFile.absolutePath else "",
                audioPath = if (sndFile.exists()) sndFile.absolutePath else ""
            )
        }

        val meta = GameMetadataExtractor.extract(context, source) ?: return CachedAssets()
        if (!dir.exists() && !dir.mkdirs()) {
            Log.w(TAG, "Could not create cache dir $dir")
            return CachedAssets()
        }

        var iconPath = ""
        var backgroundPath = ""
        var audioPath = ""
        meta.iconData?.takeIf { it.isNotEmpty() }?.let {
            runCatching { iconFile.writeBytes(it); iconPath = iconFile.absolutePath }
        }
        meta.backgroundData?.takeIf { it.isNotEmpty() }?.let {
            runCatching { bgFile.writeBytes(it); backgroundPath = bgFile.absolutePath }
        }
        meta.soundData?.takeIf { it.isNotEmpty() }?.let {
            // Stored as-is (ATRAC3). Many devices can't decode AT3, so playback is
            // best-effort; the file is still cached for any player that can.
            runCatching { sndFile.writeBytes(it); audioPath = sndFile.absolutePath }
        }
        if (meta.title.isNotBlank()) {
            runCatching { titleFile.writeText(meta.title) }
        }

        return CachedAssets(meta.title, iconPath, backgroundPath, audioPath)
    }

    private fun sanitize(name: String): String =
        name.map { if (it.isLetterOrDigit() || it == '-' || it == ' ' || it == '_') it else '_' }
            .joinToString("")
            .trim()
}
