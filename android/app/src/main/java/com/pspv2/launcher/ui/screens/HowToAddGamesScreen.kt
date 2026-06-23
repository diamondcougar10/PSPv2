package com.pspv2.launcher.ui.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/**
 * Renders the bundled HOW_TO_ADD_GAMES.txt guide from assets so players can read
 * the game-installation steps on the device. The text is shown verbatim in a
 * monospace, scrollable view to preserve its ASCII formatting.
 */
@Composable
fun HowToAddGamesScreen(onBack: () -> Unit) {
    val context = LocalContext.current
    val guide = remember {
        runCatching {
            context.assets.open("HOW_TO_ADD_GAMES.txt")
                .bufferedReader()
                .use { it.readText() }
        }.getOrDefault("Guide not found.")
    }

    Column(Modifier.fillMaxSize().background(Color(0xFF0A1A2F)).padding(20.dp)) {
        Row(
            Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text("How to Add Games", color = Color.White, fontSize = 24.sp)
            Button(onClick = onBack) { Text("Back") }
        }
        Text(
            text = guide,
            color = Color(0xDDDDE8F5),
            fontSize = 12.sp,
            fontFamily = FontFamily.Monospace,
            modifier = Modifier
                .fillMaxSize()
                .padding(top = 12.dp)
                .verticalScroll(rememberScrollState())
        )
    }
}
