package com.pspv2.launcher.media

import android.content.Context
import android.util.Log
import java.io.File

/**
 * Copies a bundled asset (such as the boot MP4) into the app cache directory and
 * returns the absolute path, because components like VideoView need a real file
 * path rather than an `assets/` stream. The copy is cached: if a file of the same
 * size already exists it is reused. Returns null if the asset is missing.
 */
fun assetToCache(context: Context, assetPath: String): String? {
    val outFile = File(context.cacheDir, assetPath.substringAfterLast('/'))
    return runCatching {
        context.assets.open(assetPath).use { input ->
            val expectedSize = input.available().toLong()
            if (outFile.exists() && outFile.length() == expectedSize) {
                return outFile.absolutePath
            }
            outFile.outputStream().use { output -> input.copyTo(output) }
        }
        outFile.absolutePath
    }.onFailure {
        Log.w("AssetCache", "Failed to stage asset: $assetPath", it)
    }.getOrNull()
}
