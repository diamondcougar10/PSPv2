package com.pspv2.launcher.ui.screens

import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.tween
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.size
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import com.pspv2.launcher.ui.rememberAssetImage
import kotlinx.coroutines.delay

/**
 * Boot logo splash, replacing the desktop IntroScreen.cpp. Fades the PSP logo in
 * then signals completion. Audio playback is wired separately via the sound bank.
 */
@Composable
fun IntroScreen(onFinished: () -> Unit) {
    var visible by remember { mutableStateOf(false) }
    val alpha by animateFloatAsState(
        targetValue = if (visible) 1f else 0f,
        animationSpec = tween(900),
        label = "introAlpha"
    )

    LaunchedEffect(Unit) {
        visible = true
        delay(2600)
        onFinished()
    }

    Box(
        Modifier.fillMaxSize().background(Color.Black),
        contentAlignment = Alignment.Center
    ) {
        val logo = rememberAssetImage("images/psp_logo.png")
        if (logo != null) {
            Image(
                bitmap = logo,
                contentDescription = "PSP",
                modifier = Modifier.size(360.dp).alpha(alpha)
            )
        }
    }
}
