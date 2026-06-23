package com.pspv2.launcher

import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.view.KeyEvent
import android.view.MotionEvent
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.result.contract.ActivityResultContracts
import androidx.activity.viewModels
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Modifier
import androidx.lifecycle.lifecycleScope
import com.pspv2.launcher.input.GamepadAction
import com.pspv2.launcher.input.GamepadInput
import com.pspv2.launcher.ui.AppScreen
import com.pspv2.launcher.ui.LauncherViewModel
import com.pspv2.launcher.ui.UiEvent
import com.pspv2.launcher.ui.screens.AboutScreen
import com.pspv2.launcher.ui.screens.CustomThemeCreatorScreen
import com.pspv2.launcher.ui.screens.GameStartupScreen
import com.pspv2.launcher.ui.screens.HowToAddGamesScreen
import com.pspv2.launcher.ui.screens.IntroScreen
import com.pspv2.launcher.ui.screens.QuickMenuOverlay
import com.pspv2.launcher.ui.screens.SetupScreen
import com.pspv2.launcher.ui.screens.ThemeSelectScreen
import com.pspv2.launcher.ui.screens.VideoIntroScreen
import com.pspv2.launcher.ui.screens.XmbScreen
import com.pspv2.launcher.ui.theme.PSPV2Theme
import kotlinx.coroutines.launch

/**
 * Single-activity host. Receives hardware key / motion events from Bluetooth gamepads
 * (Serafim and others) and keyboards, normalises them via [GamepadInput], and feeds
 * navigation actions to the [LauncherViewModel]. Replaces the SFML event loop in main.cpp.
 */
class MainActivity : ComponentActivity() {

    private val viewModel: LauncherViewModel by viewModels()

    // De-bounce flag so analog-stick motion only fires once per push.
    private var motionConsumed = false

    /** SAF folder picker for choosing the PSP ROM library directory. */
    private val pickRomFolder = registerForActivityResult(
        ActivityResultContracts.OpenDocumentTree()
    ) { uri: Uri? ->
        if (uri != null) {
            // Persist read access across reboots so we can rescan later.
            val flags = Intent.FLAG_GRANT_READ_URI_PERMISSION
            runCatching { contentResolver.takePersistableUriPermission(uri, flags) }
            viewModel.onRomFolderPicked(uri)
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        lifecycleScope.launch {
            viewModel.events.collect { event ->
                when (event) {
                    UiEvent.Exit -> finish()
                    UiEvent.PickRomFolder -> runCatching { pickRomFolder.launch(null) }
                }
            }
        }
        setContent {
            PSPV2Theme {
                val state by viewModel.state.collectAsState()
                when (state.screen) {
                    AppScreen.Intro ->
                        if (state.settings.boot_video && state.settings.boot_video_file.isNotBlank()) {
                            VideoIntroScreen(
                                assetName = state.settings.boot_video_file,
                                onFinished = viewModel::onIntroFinished
                            )
                        } else {
                            IntroScreen(onFinished = viewModel::onIntroFinished)
                        }

                    AppScreen.Setup -> SetupScreen(
                        initial = state.profile,
                        onComplete = viewModel::completeSetup
                    )

                    AppScreen.Menu,
                    AppScreen.ControllerSelect -> Box(Modifier.fillMaxSize()) {
                        XmbScreen(state, Modifier.fillMaxSize())
                        if (state.quickMenuVisible) {
                            QuickMenuOverlay(
                                selectedIndex = state.quickMenuIndex,
                                onSelect = viewModel::selectQuickMenu,
                                onConfirm = viewModel::confirmQuickMenu
                            )
                        }
                    }

                    AppScreen.GameStartup -> GameStartupScreen(
                        onFinished = viewModel::onGameStartupFinished
                    )

                    AppScreen.ThemeSelect -> ThemeSelectScreen(
                        currentTheme = state.profile.theme,
                        onSelect = viewModel::selectTheme,
                        onCreateCustom = { viewModel.goTo(AppScreen.ThemeCreate) },
                        onBack = { viewModel.goTo(AppScreen.Menu) }
                    )

                    AppScreen.ThemeCreate -> CustomThemeCreatorScreen(
                        initial = state.customTheme,
                        onSave = viewModel::saveCustomTheme,
                        onBack = { viewModel.goTo(AppScreen.ThemeSelect) }
                    )

                    AppScreen.About -> AboutScreen(
                        ppssppInstalled = viewModel.isPpssppInstalled(),
                        onHowTo = { viewModel.goTo(AppScreen.HowTo) },
                        onBack = { viewModel.goTo(AppScreen.Menu) }
                    )

                    AppScreen.HowTo -> HowToAddGamesScreen(
                        onBack = { viewModel.goTo(AppScreen.Menu) }
                    )
                }
            }
        }
    }

    override fun onKeyDown(keyCode: Int, event: KeyEvent): Boolean {
        val action = GamepadInput.fromKeyEvent(event)
        if (action != GamepadAction.NONE && routeAction(action)) return true
        return super.onKeyDown(keyCode, event)
    }

    override fun onGenericMotionEvent(event: MotionEvent): Boolean {
        if (event.source and android.view.InputDevice.SOURCE_JOYSTICK ==
            android.view.InputDevice.SOURCE_JOYSTICK &&
            event.action == MotionEvent.ACTION_MOVE
        ) {
            if (GamepadInput.isCentered(event)) {
                motionConsumed = false
            } else if (!motionConsumed) {
                val action = GamepadInput.fromMotionEvent(event)
                if (action != GamepadAction.NONE) {
                    motionConsumed = true
                    routeAction(action)
                    return true
                }
            }
        }
        return super.onGenericMotionEvent(event)
    }

    /** Routes a normalised action to the current screen. Returns true if consumed. */
    private fun routeAction(action: GamepadAction): Boolean {
        val screen = viewModel.state.value.screen
        return when (screen) {
            AppScreen.Menu, AppScreen.ControllerSelect -> {
                viewModel.onMenuAction(action)
                true
            }
            AppScreen.About, AppScreen.ThemeSelect, AppScreen.ThemeCreate, AppScreen.HowTo -> {
                if (action == GamepadAction.CANCEL) {
                    viewModel.onCancel()
                    true
                } else false
            }
            else -> false
        }
    }
}
