package com.pspv2.launcher.ui.screens

import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.foundation.Image
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
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Icon
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.scale
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.layout.ContentScale
import androidx.compose.foundation.layout.offset
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.IntOffset
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.pspv2.launcher.data.Category
import com.pspv2.launcher.data.CustomTheme
import com.pspv2.launcher.data.MenuItem
import com.pspv2.launcher.ui.UiState
import com.pspv2.launcher.ui.backgroundAssetPath
import com.pspv2.launcher.ui.iconAssetPath
import com.pspv2.launcher.ui.rememberAssetImage
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Folder

/**
 * The XMB (XrossMediaBar) home screen: a horizontal row of category icons with the
 * selected category's items listed vertically below. Compose recomposition + animated
 * floats replace the hand-written easing in the desktop Menu.cpp.
 */
@Composable
fun XmbScreen(
    state: UiState,
    modifier: Modifier = Modifier
) {
    val custom = state.profile.theme == CustomTheme.THEME_KEY
    val accent = if (custom) Color(state.customTheme.accent) else Color(0xFF6FB7FF)
    val textColor = if (custom) Color(state.customTheme.textPrimary) else Color.White

    Box(modifier = modifier.fillMaxSize().background(Color(0xFF0A1A2F))) {
        if (custom) {
            // User-authored gradient background.
            Box(
                Modifier
                    .fillMaxSize()
                    .background(
                        Brush.verticalGradient(
                            listOf(
                                Color(state.customTheme.backgroundStart),
                                Color(state.customTheme.backgroundEnd)
                            )
                        )
                    )
            )
        } else {
            // Theme background with a subtle parallax based on the selected category.
            val bg = rememberAssetImage(backgroundAssetPath(state.profile.theme))
            if (bg != null) {
                val parallax by animateFloatAsState(
                    targetValue = state.categoryIndex * -12f,
                    label = "parallax"
                )
                Image(
                    bitmap = bg,
                    contentDescription = null,
                    contentScale = ContentScale.Crop,
                    modifier = Modifier
                        .fillMaxSize()
                        .offset { IntOffset(parallax.dp.roundToPx(), 0) }
                        .scale(1.12f)
                        .alpha(0.6f)
                )
            }

            // Darkening gradient for legibility over photo backgrounds.
            Box(
                Modifier
                    .fillMaxSize()
                    .background(
                        Brush.verticalGradient(
                            listOf(Color(0xCC0A1A2F), Color(0x880A1A2F), Color(0xEE0A1A2F))
                        )
                    )
            )
        }

        Column(Modifier.fillMaxSize().padding(24.dp)) {
            StatusBar(state, textColor)
            Spacer(Modifier.height(16.dp))
            CategoryRow(state.categories, state.categoryIndex, accent)
            Spacer(Modifier.height(24.dp))
            state.currentCategory?.let { ItemList(it, state.itemIndex, accent, textColor) }
        }
    }
}

@Composable
private fun StatusBar(state: UiState, textColor: Color) {
    Row(
        Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text(
            text = state.profile.user_name.ifBlank { "PSP" },
            color = textColor,
            fontSize = 18.sp,
            fontWeight = FontWeight.SemiBold
        )
        Text(text = clockText(state), color = textColor, fontSize = 16.sp)
    }
}

@Composable
private fun CategoryRow(categories: List<Category>, selectedIndex: Int, accent: Color) {
    Row(
        Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.spacedBy(20.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        categories.forEachIndexed { index, category ->
            CategoryIcon(category, selected = index == selectedIndex)
        }
    }
}

@Composable
private fun CategoryIcon(category: Category, selected: Boolean) {
    val scale by animateFloatAsState(if (selected) 1.4f else 1f, label = "catScale")
    val alpha by animateFloatAsState(if (selected) 1f else 0.5f, label = "catAlpha")
    Column(horizontalAlignment = Alignment.CenterHorizontally) {
        val icon = rememberAssetImage(iconAssetPath(category.iconFilename))
        Box(
            Modifier.size(72.dp).scale(scale).alpha(alpha),
            contentAlignment = Alignment.Center
        ) {
            if (icon != null) {
                Image(bitmap = icon, contentDescription = category.label, Modifier.size(56.dp))
            } else {
                Icon(Icons.Filled.Folder, category.label, tint = Color.White, modifier = Modifier.size(48.dp))
            }
        }
        if (selected) {
            Text(category.label, color = Color.White, fontSize = 14.sp, fontWeight = FontWeight.Bold)
        }
    }
}

@Composable
private fun ItemList(category: Category, selectedIndex: Int, accent: Color, textColor: Color) {
    Column(verticalArrangement = Arrangement.spacedBy(6.dp)) {
        category.items.forEachIndexed { index, item ->
            ItemRow(item, selected = index == selectedIndex, accent = accent, textColor = textColor)
        }
    }
}

@Composable
private fun ItemRow(item: MenuItem, selected: Boolean, accent: Color, textColor: Color) {
    val alpha by animateFloatAsState(if (selected) 1f else 0.55f, label = "itemAlpha")
    Row(
        Modifier
            .fillMaxWidth()
            .alpha(alpha)
            .clip(RoundedCornerShape(8.dp))
            .then(if (selected) Modifier.background(accent.copy(alpha = 0.22f)) else Modifier)
            .padding(horizontal = 12.dp, vertical = 8.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        val icon = rememberAssetImage(iconAssetPath(item.iconFilename))
        if (icon != null) {
            Image(bitmap = icon, contentDescription = item.label, Modifier.size(32.dp))
            Spacer(Modifier.width(12.dp))
        }
        Column {
            Text(
                item.label,
                color = if (selected) accent else textColor,
                fontSize = 18.sp,
                fontWeight = if (selected) FontWeight.Bold else FontWeight.Normal
            )
            Text(itemTypeLabel(item.type), color = textColor.copy(alpha = 0.67f), fontSize = 12.sp)
        }
    }
}

private fun itemTypeLabel(type: String): String = when (type) {
    "psp_iso", "psp_eboot" -> "PSP Game"
    "web_url" -> "Web Link"
    "folder" -> "Folder"
    "theme_select" -> "Themes"
    "theme_create" -> "Themes"
    "scan_roms" -> "System"
    "how_to_games" -> "Help"
    "about" -> "System"
    "factory_reset" -> "System"
    else -> type
}

private fun clockText(state: UiState): String {
    if (!state.profile.show_clock) return ""
    val now = java.util.Calendar.getInstance()
    val pattern = when {
        state.profile.show_date && state.profile.use_24_hour_format -> "EEE d MMM  HH:mm"
        state.profile.show_date -> "EEE d MMM  h:mm a"
        state.profile.use_24_hour_format -> "HH:mm"
        else -> "h:mm a"
    }
    return java.text.SimpleDateFormat(pattern, java.util.Locale.getDefault()).format(now.time)
}
