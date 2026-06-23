package com.pspv2.launcher.ui.theme

import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Typography
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.graphics.Color

// PSP XMB-inspired palette.
private val PspBlue = Color(0xFF1B4F8C)
private val PspLightBlue = Color(0xFF6FB7FF)
private val PspBackground = Color(0xFF0A1A2F)
private val PspSurface = Color(0xFF12263F)

private val PspColors = darkColorScheme(
    primary = PspLightBlue,
    onPrimary = Color.White,
    secondary = PspBlue,
    background = PspBackground,
    onBackground = Color.White,
    surface = PspSurface,
    onSurface = Color.White,
)

@Composable
fun PSPV2Theme(content: @Composable () -> Unit) {
    MaterialTheme(
        colorScheme = PspColors,
        typography = Typography(),
        content = content
    )
}
