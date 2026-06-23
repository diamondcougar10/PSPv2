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
import com.pspv2.launcher.ui.screens.ImportStatusBanner
import com.pspv2.launcher.ui.screens.IntroScreen
import com.pspv2.launcher.ui.screens.PpssppMissingDialog
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

    // True while we are dispatching a synthetic D-pad key for Compose focus, so the
    // re-entrant key callback ignores it instead of re-injecting (which would loop).
    private var injectingFocusKey = false

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

    /** SAF file picker for importing a single downloaded ROM or .zip. */
    private val pickRomFile = registerForActivityResult(
        ActivityResultContracts.OpenDocument()
    ) { uri: Uri? ->
        if (uri != null) {
            runCatching {
                contentResolver.takePersistableUriPermission(uri, Intent.FLAG_GRANT_READ_URI_PERMISSION)
            }
            viewModel.onRomFilePicked(uri)
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
                    UiEvent.PickRomFile -> runCatching {
                        // ROM downloads come in many archive formats (zip/7z/rar/tar/gz…);
                        // "*/*" keeps the picker from hiding files some providers report odd
                        // or missing MIME types for, while the named types aid discovery.
                        pickRomFile.launch(arrayOf(
                            "application/zip",
                            "application/x-7z-compressed",
                            "application/x-rar-compressed",
                            "application/vnd.rar",
                            "application/x-tar",
                            "application/gzip",
                            "application/x-bzip2",
                            "application/x-xz",
                            "application/octet-stream",
                            "*/*"
                        ))
                    }
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
                        state.importStatus?.let {
                            ImportStatusBanner(
                                status = it,
                                modifier = Modifier.align(androidx.compose.ui.Alignment.BottomCenter)
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
                        onBack = { viewModel.goTo(AppScreen.Menu) },
                        scrollNudges = viewModel.scrollNudges
                    )
                }

                // Global prompt: shown over any screen when a game launch is attempted
                // without PPSSPP installed.
                if (state.ppssppMissing) {
                    PpssppMissingDialog(
                        onInstall = viewModel::installPpsspp,
                        onDismiss = viewModel::dismissPpssppPrompt
                    )
                }
            }
        }
    }

    override fun onKeyDown(keyCode: Int, event: KeyEvent): Boolean {
        // Ignore the synthetic D-pad events we inject for Compose focus traversal,
        // otherwise we would map them straight back into another injection (loop).
        if (injectingFocusKey) return super.onKeyDown(keyCode, event)
        val action = GamepadInput.fromKeyEvent(event)
        if (action != GamepadAction.NONE && routeAction(action)) return true
        return super.onKeyDown(keyCode, event)
    }

    override fun onGenericMotionEvent(event: MotionEvent): Boolean {
        if (injectingFocusKey) return super.onGenericMotionEvent(event)
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
        val state = viewModel.state.value
        // The "install PPSSPP" prompt sits above everything; it captures input first.
        if (state.ppssppMissing) {
            return when (action) {
                GamepadAction.CONFIRM -> { viewModel.installPpsspp(); true }
                GamepadAction.CANCEL -> { viewModel.dismissPpssppPrompt(); true }
                else -> true // swallow navigation so it doesn't move the menu behind the dialog
            }
        }
        return when (val screen = state.screen) {
            AppScreen.Menu, AppScreen.ControllerSelect -> {
                viewModel.onMenuAction(action)
                true
            }

            // Form screens (Setup, theme picker/creator, About) are driven by Compose's
            // native focus system so D-pad, analog stick AND the A button all operate
            // text fields, sliders and buttons. We translate every gamepad action into a
            // standard D-pad key event and inject it so focus traversal / activation work.
            AppScreen.Setup,
            AppScreen.ThemeSelect,
            AppScreen.ThemeCreate,
            AppScreen.About -> {
                routeFocusScreen(action, screen)
                true
            }

            // Long-text screen: D-pad / stick scrolls, confirm or cancel returns.
            AppScreen.HowTo -> {
                when (action) {
                    GamepadAction.UP -> viewModel.nudgeScroll(-SCROLL_STEP_DP)
                    GamepadAction.DOWN -> viewModel.nudgeScroll(SCROLL_STEP_DP)
                    GamepadAction.CANCEL, GamepadAction.CONFIRM -> viewModel.goTo(AppScreen.Menu)
                    else -> Unit
                }
                true
            }

            // Animations: any confirm / cancel skips straight ahead.
            AppScreen.Intro -> {
                if (action == GamepadAction.CONFIRM || action == GamepadAction.CANCEL) {
                    viewModel.skipIntro()
                }
                true
            }
            AppScreen.GameStartup -> {
                if (action == GamepadAction.CONFIRM || action == GamepadAction.CANCEL) {
                    viewModel.skipGameStartup()
                }
                true
            }
        }
    }

    /**
     * Drives a Compose-focus form screen with the gamepad. Cancel maps to the screen's
     * natural "back" target; everything else is injected as a D-pad key so the focused
     * control (button, slider, text field) reacts exactly as it would to a TV remote.
     */
    private fun routeFocusScreen(action: GamepadAction, screen: AppScreen) {
        when (action) {
            GamepadAction.CANCEL -> when (screen) {
                AppScreen.ThemeCreate -> viewModel.goTo(AppScreen.ThemeSelect)
                AppScreen.Setup -> Unit // first-time setup has no back
                else -> viewModel.goTo(AppScreen.Menu)
            }
            GamepadAction.UP -> injectFocusKey(KeyEvent.KEYCODE_DPAD_UP)
            GamepadAction.DOWN -> injectFocusKey(KeyEvent.KEYCODE_DPAD_DOWN)
            GamepadAction.LEFT -> injectFocusKey(KeyEvent.KEYCODE_DPAD_LEFT)
            GamepadAction.RIGHT -> injectFocusKey(KeyEvent.KEYCODE_DPAD_RIGHT)
            GamepadAction.CONFIRM -> injectFocusKey(KeyEvent.KEYCODE_DPAD_CENTER)
            GamepadAction.MENU, GamepadAction.NONE -> Unit
        }
    }

    /** Dispatches a synthetic D-pad key event so Compose handles focus/activation. */
    private fun injectFocusKey(keyCode: Int) {
        val now = android.os.SystemClock.uptimeMillis()
        val decor = window.decorView
        injectingFocusKey = true
        try {
            decor.dispatchKeyEvent(KeyEvent(now, now, KeyEvent.ACTION_DOWN, keyCode, 0))
            decor.dispatchKeyEvent(KeyEvent(now, now, KeyEvent.ACTION_UP, keyCode, 0))
        } finally {
            injectingFocusKey = false
        }
    }

    private companion object {
        const val SCROLL_STEP_DP = 120
    }
}
