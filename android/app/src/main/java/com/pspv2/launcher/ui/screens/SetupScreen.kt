package com.pspv2.launcher.ui.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material3.Button
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.material3.TextField
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.pspv2.launcher.data.UserProfile

/**
 * First-time setup wizard, replacing SetupScreen.cpp: choose a username and clock
 * preferences. Theme can be changed later from the Settings category.
 */
@Composable
fun SetupScreen(
    initial: UserProfile,
    onComplete: (UserProfile) -> Unit
) {
    var name by remember { mutableStateOf(initial.user_name) }
    var showClock by remember { mutableStateOf(initial.show_clock) }
    var showDate by remember { mutableStateOf(initial.show_date) }
    var use24h by remember { mutableStateOf(initial.use_24_hour_format) }

    Box(
        Modifier.fillMaxSize().background(Color(0xFF0A1A2F)).padding(32.dp),
        contentAlignment = Alignment.Center
    ) {
        Column(
            Modifier.width(420.dp),
            verticalArrangement = Arrangement.spacedBy(16.dp)
        ) {
            Text("Welcome", color = Color.White, fontSize = 28.sp)
            Text("Let's set up your PSP.", color = Color(0xAAFFFFFF), fontSize = 16.sp)

            TextField(
                value = name,
                onValueChange = { name = it },
                label = { Text("Username") },
                singleLine = true,
                keyboardOptions = KeyboardOptions(imeAction = ImeAction.Done),
                modifier = Modifier.fillMaxWidth()
            )

            SettingToggle("Show clock", showClock) { showClock = it }
            SettingToggle("Show date", showDate) { showDate = it }
            SettingToggle("24-hour time", use24h) { use24h = it }

            Button(
                onClick = {
                    onComplete(
                        initial.copy(
                            user_name = name.trim().ifBlank { "PSP" },
                            show_clock = showClock,
                            show_date = showDate,
                            use_24_hour_format = use24h,
                            first_time_setup = false
                        )
                    )
                },
                modifier = Modifier.fillMaxWidth()
            ) {
                Text("Continue")
            }
        }
    }
}

@Composable
private fun SettingToggle(label: String, checked: Boolean, onChange: (Boolean) -> Unit) {
    androidx.compose.foundation.layout.Row(
        Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text(label, color = Color.White, fontSize = 16.sp)
        Switch(checked = checked, onCheckedChange = onChange)
    }
}
