package com.pspv2.launcher.ui.screens

import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.foundation.lazy.grid.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.pspv2.launcher.ui.backgroundAssetPath
import com.pspv2.launcher.ui.rememberAssetImage

/**
 * Theme picker, replacing ThemeSelector.cpp. Lists the background images bundled in
 * assets/Backgrounds and lets the user pick one; selection persists via the ViewModel.
 */
@Composable
fun ThemeSelectScreen(
    currentTheme: String,
    onSelect: (String) -> Unit,
    onCreateCustom: () -> Unit,
    onBack: () -> Unit
) {
    val context = LocalContext.current
    val themes = remember {
        runCatching {
            context.assets.list("Backgrounds")?.filter {
                it.endsWith(".png", true) || it.endsWith(".jpg", true) || it.endsWith(".bmp", true)
            }?.sorted() ?: emptyList()
        }.getOrDefault(emptyList())
    }

    Column(Modifier.fillMaxSize().background(Color(0xFF0A1A2F)).padding(24.dp)) {
        Row(Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
            Column(Modifier.weight(1f)) {
                Text("Themes", color = Color.White, fontSize = 26.sp)
                Text("Select a background", color = Color(0xAAFFFFFF), fontSize = 14.sp)
            }
            Button(
                onClick = onCreateCustom,
                colors = ButtonDefaults.buttonColors(containerColor = Color(0xFF00B4FF))
            ) { Text("Create custom") }
        }

        LazyVerticalGrid(
            columns = GridCells.Adaptive(180.dp),
            modifier = Modifier.fillMaxSize().padding(top = 16.dp)
        ) {
            items(themes) { theme ->
                val bitmap = rememberAssetImage(backgroundAssetPath(theme))
                Box(
                    Modifier
                        .padding(8.dp)
                        .fillMaxWidth()
                        .height(110.dp)
                        .clip(RoundedCornerShape(10.dp))
                        .border(
                            width = if (theme == currentTheme) 3.dp else 0.dp,
                            color = Color(0xFF6FB7FF),
                            shape = RoundedCornerShape(10.dp)
                        )
                        .clickable {
                            onSelect(theme)
                            onBack()
                        }
                ) {
                    if (bitmap != null) {
                        Image(bitmap, theme, Modifier.fillMaxSize(), contentScale = ContentScale.Crop)
                    }
                }
            }
        }
    }
}
