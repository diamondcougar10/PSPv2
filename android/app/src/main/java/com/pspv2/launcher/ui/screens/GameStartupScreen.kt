package com.pspv2.launcher.ui.screens

import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.size
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.unit.dp
import com.pspv2.launcher.ui.rememberAssetImage
import kotlinx.coroutines.delay

/**
 * The PSP "Sony Computer Entertainment" boot animation shown before a game launches,
 * replacing GameStartupScreen.cpp. After the animation it calls [onFinished], which
 * hands the game off to PPSSPP.
 */
@Composable
fun GameStartupScreen(onFinished: () -> Unit) {
    LaunchedEffect(Unit) {
        delay(2200)
        onFinished()
    }

    Box(
        Modifier.fillMaxSize().background(Color.Black),
        contentAlignment = Alignment.Center
    ) {
        val startup = rememberAssetImage("images/PSP-STARTUP.png")
        val alpha by animateFloatAsState(if (startup != null) 1f else 0f, label = "startupAlpha")
        if (startup != null) {
            Image(
                bitmap = startup,
                contentDescription = null,
                contentScale = ContentScale.Fit,
                modifier = Modifier.size(420.dp).alpha(alpha)
            )
        }
    }
}
