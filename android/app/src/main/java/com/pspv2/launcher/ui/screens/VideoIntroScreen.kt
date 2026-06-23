package com.pspv2.launcher.ui.screens

import android.media.MediaPlayer
import android.widget.VideoView
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.viewinterop.AndroidView
import com.pspv2.launcher.media.assetToCache

/**
 * Plays the bundled MP4 boot video (e.g. assets/intro/CurpheyMade.mp4) full-screen,
 * the Android port of the desktop intro movie. [VideoView] can't read straight from
 * the APK's assets, so the clip is copied into the cache directory once and played
 * from there. When playback ends — or if the file is missing / fails — [onFinished]
 * fires so the launcher proceeds to the XMB.
 */
@Composable
fun VideoIntroScreen(assetName: String, onFinished: () -> Unit) {
    val context = LocalContext.current
    var finished by remember { mutableStateOf(false) }

    fun finishOnce() {
        if (!finished) {
            finished = true
            onFinished()
        }
    }

    // Resolve the playable file path off the composition's first frame.
    val videoPath = remember(assetName) { assetToCache(context, "intro/$assetName") }

    LaunchedEffect(videoPath) {
        if (videoPath == null) finishOnce()
    }

    Box(Modifier.fillMaxSize().background(Color.Black)) {
        if (videoPath != null) {
            AndroidView(
                factory = { ctx ->
                    VideoView(ctx).apply {
                        setVideoPath(videoPath)
                        setOnPreparedListener { mp: MediaPlayer ->
                            mp.isLooping = false
                            start()
                        }
                        setOnCompletionListener { finishOnce() }
                        setOnErrorListener { _, _, _ -> finishOnce(); true }
                    }
                },
                modifier = Modifier.fillMaxSize()
            )
        }
    }
}
