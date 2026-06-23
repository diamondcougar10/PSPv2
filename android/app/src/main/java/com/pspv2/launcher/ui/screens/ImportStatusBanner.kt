package com.pspv2.launcher.ui.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.Icon
import androidx.compose.material3.Text
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.CheckCircle
import androidx.compose.material.icons.filled.Error
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.pspv2.launcher.ui.ImportStatus

/**
 * A small overlay banner that reports ROM-import progress and the final result,
 * so the user can see exactly what is happening while a download is installed.
 */
@Composable
fun ImportStatusBanner(status: ImportStatus, modifier: Modifier = Modifier) {
    val accent = when {
        status.isError -> Color(0xFFE05656)
        status.busy -> Color(0xFF6FB7FF)
        else -> Color(0xFF5CC98C)
    }
    Box(modifier = modifier.fillMaxWidth().padding(24.dp), contentAlignment = Alignment.BottomCenter) {
        Row(
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.Start,
            modifier = Modifier
                .clip(RoundedCornerShape(14.dp))
                .background(Color(0xF2102236))
                .padding(horizontal = 20.dp, vertical = 14.dp)
        ) {
            when {
                status.busy -> CircularProgressIndicator(
                    color = accent,
                    strokeWidth = 3.dp,
                    modifier = Modifier.size(22.dp)
                )
                status.isError -> Icon(Icons.Filled.Error, contentDescription = null, tint = accent, modifier = Modifier.size(24.dp))
                else -> Icon(Icons.Filled.CheckCircle, contentDescription = null, tint = accent, modifier = Modifier.size(24.dp))
            }
            Spacer(Modifier.width(14.dp))
            Text(
                text = status.message,
                color = Color.White,
                fontSize = 16.sp,
                fontWeight = FontWeight.Medium
            )
        }
    }
}
