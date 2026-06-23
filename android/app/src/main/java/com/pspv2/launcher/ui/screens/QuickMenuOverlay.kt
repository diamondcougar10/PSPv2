package com.pspv2.launcher.ui.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.clickable
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.pspv2.launcher.ui.QuickMenuOption

/**
 * Translucent options overlay drawn on top of the XMB, the Android port of
 * [src/QuickMenu.cpp]. Opened with the Start / Menu button; navigation is driven
 * by the gamepad through the ViewModel, but the rows are also tap-friendly.
 */
@Composable
fun QuickMenuOverlay(
    selectedIndex: Int,
    onSelect: (Int) -> Unit,
    onConfirm: () -> Unit,
    modifier: Modifier = Modifier
) {
    Box(
        modifier = modifier
            .fillMaxSize()
            .background(Color(0xCC000510)),
        contentAlignment = Alignment.Center
    ) {
        Column(
            Modifier
                .width(360.dp)
                .clip(RoundedCornerShape(14.dp))
                .background(Color(0xF21A2A44))
                .padding(vertical = 18.dp)
        ) {
            Text(
                text = "Options",
                color = Color(0xFF8FD0FF),
                fontSize = 14.sp,
                fontWeight = FontWeight.Bold,
                modifier = Modifier.padding(start = 24.dp, bottom = 12.dp)
            )
            QuickMenuOption.entries.forEachIndexed { index, option ->
                val selected = index == selectedIndex
                Box(
                    Modifier
                        .fillMaxWidth()
                        .background(if (selected) Color(0x3300B4FF) else Color.Transparent)
                        .clickable {
                            onSelect(index)
                            onConfirm()
                        }
                        .padding(horizontal = 24.dp, vertical = 12.dp)
                ) {
                    Text(
                        text = option.label,
                        color = if (selected) Color.White else Color(0xFFB8C6DA),
                        fontSize = 18.sp,
                        fontWeight = if (selected) FontWeight.Bold else FontWeight.Normal
                    )
                }
            }
        }
    }
}
