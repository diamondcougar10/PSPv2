package com.pspv2.launcher.ui.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.window.Dialog

/**
 * Friendly prompt shown when the user selects a game but no PPSSPP build is
 * installed. Offers a one-tap jump to the store listing so they can install it,
 * or dismiss and carry on. Detection happens automatically, so this only appears
 * when PPSSPP genuinely is not present on the device.
 */
@Composable
fun PpssppMissingDialog(
    onInstall: () -> Unit,
    onDismiss: () -> Unit
) {
    Dialog(onDismissRequest = onDismiss) {
        Column(
            modifier = Modifier
                .widthIn(max = 420.dp)
                .clip(RoundedCornerShape(16.dp))
                .background(Color(0xF2101C2C))
                .padding(24.dp),
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            Text(
                text = "PPSSPP not found",
                color = Color.White,
                fontSize = 22.sp,
                fontWeight = FontWeight.Bold
            )
            Text(
                text = "PSPV2 launches your PSP games through the PPSSPP " +
                    "emulator, but it isn't installed yet. Install PPSSPP, then " +
                    "select your game again.",
                color = Color(0xFFB9C6D8),
                fontSize = 15.sp,
                textAlign = TextAlign.Center,
                modifier = Modifier.padding(top = 12.dp, bottom = 24.dp)
            )
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(12.dp)
            ) {
                DialogButton(
                    label = "\u25CB  Not now",
                    background = Color(0x33FFFFFF),
                    textColor = Color.White,
                    modifier = Modifier.weight(1f),
                    onClick = onDismiss
                )
                DialogButton(
                    label = "\u2715  Install PPSSPP",
                    background = Color(0xFF2E7DEF),
                    textColor = Color.White,
                    modifier = Modifier.weight(1f),
                    onClick = onInstall
                )
            }
        }
    }
}

@Composable
private fun DialogButton(
    label: String,
    background: Color,
    textColor: Color,
    modifier: Modifier = Modifier,
    onClick: () -> Unit
) {
    Box(
        modifier = modifier
            .clip(RoundedCornerShape(10.dp))
            .background(background)
            .clickable(onClick = onClick)
            .padding(vertical = 12.dp),
        contentAlignment = Alignment.Center
    ) {
        Text(
            text = label,
            color = textColor,
            fontSize = 15.sp,
            fontWeight = FontWeight.SemiBold
        )
    }
}
