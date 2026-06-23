package com.pspv2.launcher.ui.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Slider
import androidx.compose.material3.SliderDefaults
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.focus.focusRequester
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.pspv2.launcher.data.CustomTheme

/**
 * The Android port of CustomThemeCreator.cpp. Instead of the desktop's joystick-driven
 * RGB sliders, this uses touch-friendly Compose [Slider]s for the background gradient,
 * accent and text colours, with a live preview pane on the right. Saving makes the
 * theme active immediately.
 */
@Composable
fun CustomThemeCreatorScreen(
    initial: CustomTheme,
    onSave: (CustomTheme) -> Unit,
    onBack: () -> Unit
) {
    var name by remember { mutableStateOf(initial.name) }
    var bgStart by remember { mutableStateOf(Color(initial.backgroundStart)) }
    var bgEnd by remember { mutableStateOf(Color(initial.backgroundEnd)) }
    var accent by remember { mutableStateOf(Color(initial.accent)) }
    var textPrimary by remember { mutableStateOf(Color(initial.textPrimary)) }
    val initialFocus = remember { FocusRequester() }
    LaunchedEffect(Unit) { runCatching { initialFocus.requestFocus() } }

    Row(Modifier.fillMaxSize().background(Color(0xFF0A1A2F)).padding(24.dp)) {
        // ---- Editor column ----
        Column(
            Modifier
                .weight(1f)
                .fillMaxSize()
                .verticalScroll(rememberScrollState())
                .padding(end = 24.dp)
        ) {
            Text("Custom Theme", color = Color.White, fontSize = 26.sp, fontWeight = FontWeight.Bold)
            Spacer(Modifier.height(12.dp))

            OutlinedTextField(
                value = name,
                onValueChange = { name = it },
                label = { Text("Theme name") },
                singleLine = true,
                modifier = Modifier.fillMaxWidth()
            )
            Spacer(Modifier.height(16.dp))

            ColorEditor("Background top", bgStart) { bgStart = it }
            ColorEditor("Background bottom", bgEnd) { bgEnd = it }
            ColorEditor("Accent", accent) { accent = it }
            ColorEditor("Text", textPrimary) { textPrimary = it }

            Spacer(Modifier.height(20.dp))
            Row(horizontalArrangement = Arrangement.spacedBy(12.dp)) {
                Button(
                    onClick = {
                        onSave(
                            CustomTheme(
                                name = name.ifBlank { "Custom Theme" },
                                backgroundStart = bgStart.toArgbLong(),
                                backgroundEnd = bgEnd.toArgbLong(),
                                accent = accent.toArgbLong(),
                                textPrimary = textPrimary.toArgbLong()
                            )
                        )
                    },
                    colors = ButtonDefaults.buttonColors(containerColor = accent)
                ) { Text("Save & Apply") }

                Button(
                    onClick = onBack,
                    modifier = Modifier.focusRequester(initialFocus),
                    colors = ButtonDefaults.buttonColors(containerColor = Color(0xFF33415A))
                ) { Text("Cancel") }
            }
        }

        // ---- Live preview column ----
        Box(
            Modifier
                .weight(1f)
                .fillMaxSize()
                .clip(RoundedCornerShape(16.dp))
                .background(Brush.verticalGradient(listOf(bgStart, bgEnd)))
                .padding(20.dp)
        ) {
            Column {
                Text("Preview", color = textPrimary, fontSize = 22.sp, fontWeight = FontWeight.Bold)
                Spacer(Modifier.height(16.dp))
                repeat(4) { i ->
                    val selected = i == 1
                    Box(
                        Modifier
                            .fillMaxWidth()
                            .padding(vertical = 6.dp)
                            .clip(RoundedCornerShape(8.dp))
                            .background(if (selected) accent.copy(alpha = 0.30f) else Color.Transparent)
                            .padding(horizontal = 14.dp, vertical = 10.dp)
                    ) {
                        Text(
                            text = "Menu item ${i + 1}",
                            color = if (selected) accent else textPrimary.copy(alpha = 0.8f),
                            fontSize = 18.sp,
                            fontWeight = if (selected) FontWeight.Bold else FontWeight.Normal
                        )
                    }
                }
            }
        }
    }
}

@Composable
private fun ColorEditor(label: String, color: Color, onChange: (Color) -> Unit) {
    Column(Modifier.padding(vertical = 8.dp)) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Box(
                Modifier
                    .width(28.dp)
                    .height(28.dp)
                    .clip(RoundedCornerShape(6.dp))
                    .background(color)
            )
            Spacer(Modifier.width(10.dp))
            Text(label, color = Color.White, fontSize = 16.sp, fontWeight = FontWeight.SemiBold)
        }
        ChannelSlider("R", color.red) { onChange(color.copy(red = it)) }
        ChannelSlider("G", color.green) { onChange(color.copy(green = it)) }
        ChannelSlider("B", color.blue) { onChange(color.copy(blue = it)) }
    }
}

@Composable
private fun ChannelSlider(label: String, value: Float, onChange: (Float) -> Unit) {
    Row(verticalAlignment = Alignment.CenterVertically) {
        Text(label, color = Color(0xFFB8C6DA), fontSize = 13.sp, modifier = Modifier.width(18.dp))
        Slider(
            value = value,
            onValueChange = onChange,
            valueRange = 0f..1f,
            colors = SliderDefaults.colors(
                thumbColor = Color(0xFF6FB7FF),
                activeTrackColor = Color(0xFF6FB7FF)
            ),
            modifier = Modifier.weight(1f)
        )
        Text(
            text = (value * 255).toInt().toString(),
            color = Color(0xFFB8C6DA),
            fontSize = 13.sp,
            modifier = Modifier.width(34.dp)
        )
    }
}

/** Pack a Compose [Color] into an ARGB long for serialization. */
private fun Color.toArgbLong(): Long {
    val a = (alpha * 255).toInt() and 0xFF
    val r = (red * 255).toInt() and 0xFF
    val g = (green * 255).toInt() and 0xFF
    val b = (blue * 255).toInt() and 0xFF
    return ((a.toLong() shl 24) or (r.toLong() shl 16) or (g.toLong() shl 8) or b.toLong())
}
