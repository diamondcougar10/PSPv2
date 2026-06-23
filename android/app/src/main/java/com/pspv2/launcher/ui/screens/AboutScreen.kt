package com.pspv2.launcher.ui.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/** Simple About screen, replacing AboutScreen.cpp. */
@Composable
fun AboutScreen(ppssppInstalled: Boolean, onHowTo: () -> Unit, onBack: () -> Unit) {
    Box(
        Modifier.fillMaxSize().background(Color(0xFF0A1A2F)).padding(32.dp),
        contentAlignment = Alignment.Center
    ) {
        Column(
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.spacedBy(14.dp)
        ) {
            Text("PSPV2", color = Color.White, fontSize = 32.sp)
            Text(
                "A PSP-style XMB launcher for Android.\nGames run through the installed PPSSPP app.",
                color = Color(0xCCFFFFFF),
                fontSize = 16.sp,
                textAlign = TextAlign.Center
            )
            Text(
                if (ppssppInstalled) "PPSSPP: detected" else "PPSSPP: not installed",
                color = if (ppssppInstalled) Color(0xFF7CFF9E) else Color(0xFFFF7C7C),
                fontSize = 16.sp
            )
            Button(onClick = onHowTo) { Text("How to Add Games") }
            Button(onClick = onBack) { Text("Back") }
        }
    }
}
