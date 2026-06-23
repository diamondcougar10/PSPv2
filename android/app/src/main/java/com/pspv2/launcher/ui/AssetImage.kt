package com.pspv2.launcher.ui

import android.content.Context
import android.graphics.BitmapFactory
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.graphics.ImageBitmap
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.platform.LocalContext

/**
 * Loads an image bundled under assets/ (e.g. "Backgrounds/Background3.png",
 * "Icons/psp game.png") into a Compose ImageBitmap. Returns null while loading or
 * if the asset is missing, so callers can fall back to a placeholder.
 */
@Composable
fun rememberAssetImage(assetPath: String?): ImageBitmap? {
    val context = LocalContext.current
    var bitmap by remember(assetPath) { mutableStateOf<ImageBitmap?>(null) }
    LaunchedEffect(assetPath) {
        bitmap = if (assetPath.isNullOrBlank()) null else loadAssetBitmap(context, assetPath)
    }
    return bitmap
}

private fun loadAssetBitmap(context: Context, assetPath: String): ImageBitmap? {
    return runCatching {
        context.assets.open(assetPath).use { stream ->
            BitmapFactory.decodeStream(stream)?.asImageBitmap()
        }
    }.getOrNull()
}

/**
 * Loads an image from an absolute filesystem path (e.g. a cached game ICON0.PNG under
 * filesDir/previews). Returns null while loading or if the file is missing/unreadable.
 */
@Composable
fun rememberFileImage(filePath: String?): ImageBitmap? {
    var bitmap by remember(filePath) { mutableStateOf<ImageBitmap?>(null) }
    LaunchedEffect(filePath) {
        bitmap = if (filePath.isNullOrBlank()) null else loadFileBitmap(filePath)
    }
    return bitmap
}

private fun loadFileBitmap(filePath: String): ImageBitmap? {
    return runCatching {
        val file = java.io.File(filePath)
        if (!file.exists()) null else BitmapFactory.decodeFile(filePath)?.asImageBitmap()
    }.getOrNull()
}

/** Resolves a bare icon filename to its assets/Icons/ path. */
fun iconAssetPath(filename: String): String? =
    if (filename.isBlank()) null else "Icons/$filename"

/** Resolves a theme background filename to its assets/Backgrounds/ path. */
fun backgroundAssetPath(filename: String): String? =
    if (filename.isBlank()) null else "Backgrounds/$filename"
