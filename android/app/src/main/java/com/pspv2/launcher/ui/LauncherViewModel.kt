package com.pspv2.launcher.ui

import android.app.Application
import android.net.Uri
import androidx.core.net.toUri
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.pspv2.launcher.audio.UiSoundBank
import com.pspv2.launcher.data.AppSettings
import com.pspv2.launcher.data.Category
import com.pspv2.launcher.data.ConfigRepository
import com.pspv2.launcher.data.CustomTheme
import com.pspv2.launcher.data.MenuConfig
import com.pspv2.launcher.data.MenuItem
import com.pspv2.launcher.data.RomScanner
import com.pspv2.launcher.data.UserProfile
import com.pspv2.launcher.input.GamepadAction
import com.pspv2.launcher.launch.GameLauncher
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asSharedFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

/** Top-level screens, mirroring the C++ AppState enum in main.cpp. */
enum class AppScreen { Intro, Setup, Menu, ControllerSelect, GameStartup, ThemeSelect, ThemeCreate, About, HowTo }

/** One-shot events emitted to the host activity. */
enum class UiEvent { Exit, PickRomFolder }

/** Options available in the XMB quick menu overlay (Android port of QuickMenu.cpp). */
enum class QuickMenuOption(val label: String) {
    Resume("Resume"),
    ChangeTheme("Change Theme"),
    About("About"),
    Exit("Exit PSPV2")
}

data class UiState(
    val screen: AppScreen = AppScreen.Intro,
    val profile: UserProfile = UserProfile(),
    val settings: AppSettings = AppSettings(),
    val categories: List<Category> = emptyList(),
    val categoryIndex: Int = 0,
    val itemIndex: Int = 0,
    val pendingLaunch: MenuItem? = null,
    val quickMenuVisible: Boolean = false,
    val quickMenuIndex: Int = 0,
    val customTheme: CustomTheme = CustomTheme()
) {
    val currentCategory: Category? get() = categories.getOrNull(categoryIndex)
    val currentItem: MenuItem? get() = currentCategory?.items?.getOrNull(itemIndex)
}

/**
 * Drives the whole launcher: owns the XMB navigation state and routes confirm
 * actions to the [GameLauncher]. Replaces the giant state machine in main.cpp.
 */
class LauncherViewModel(app: Application) : AndroidViewModel(app) {

    private val repo = ConfigRepository(app)
    private val launcher = GameLauncher(app)
    private val sounds = UiSoundBank(app)

    private val _state = MutableStateFlow(UiState())
    val state: StateFlow<UiState> = _state.asStateFlow()

    /** One-shot UI events the host activity reacts to (e.g. exit). */
    private val _events = MutableSharedFlow<UiEvent>(extraBufferCapacity = 1)
    val events = _events.asSharedFlow()

    init {
        val menu: MenuConfig = repo.loadMenu()
        val profile = repo.loadProfile()
        val settings = repo.loadSettings()
        val customTheme = repo.loadCustomTheme()
        applySoundSettings(settings)
        _state.update {
            it.copy(
                profile = profile,
                settings = settings,
                categories = menu.categories,
                customTheme = customTheme
            )
        }
        // If the user previously chose a ROM folder, rescan it on launch so newly
        // added games show up without re-picking the folder.
        if (settings.games_tree_uri.isNotBlank()) {
            rescanSavedRomFolder(settings.games_tree_uri)
        }
    }

    private fun rescanSavedRomFolder(treeUriString: String) {
        val treeUri = runCatching { treeUriString.toUri() }.getOrNull() ?: return
        viewModelScope.launch {
            val games = withContext(Dispatchers.IO) {
                runCatching { RomScanner.scan(getApplication(), treeUri) }.getOrDefault(emptyList())
            }
            if (games.isNotEmpty()) {
                _state.update { st ->
                    val updated = st.categories.map { cat ->
                        if (cat.id == "games") cat.copy(items = games) else cat
                    }
                    st.copy(categories = updated)
                }
            }
        }
    }

    private fun applySoundSettings(settings: AppSettings) {
        sounds.enabled = settings.ui_sounds
        sounds.useV150 = settings.use_v150_sounds
    }

    /** Called when the intro animation finishes. */
    fun onIntroFinished() {
        sounds.playOpening()
        val next = if (_state.value.profile.first_time_setup) AppScreen.Setup else AppScreen.Menu
        _state.update { it.copy(screen = next) }
    }

    fun completeSetup(profile: UserProfile) {
        val saved = profile.copy(first_time_setup = false)
        repo.saveProfile(saved)
        _state.update { it.copy(profile = saved, screen = AppScreen.Menu) }
    }

    fun goTo(screen: AppScreen) = _state.update { it.copy(screen = screen) }

    fun selectTheme(themeFilename: String) {
        val saved = _state.value.profile.copy(theme = themeFilename)
        repo.saveProfile(saved)
        _state.update { it.copy(profile = saved) }
    }

    /** Open the custom theme creator, seeded with the current saved custom theme. */
    fun openThemeCreator() {
        sounds.playDecide()
        goTo(AppScreen.ThemeCreate)
    }

    /** Persist a newly authored custom theme and make it the active theme. */
    fun saveCustomTheme(theme: CustomTheme) {
        sounds.playSystemOk()
        repo.saveCustomTheme(theme)
        val savedProfile = _state.value.profile.copy(theme = CustomTheme.THEME_KEY)
        repo.saveProfile(savedProfile)
        _state.update { it.copy(customTheme = theme, profile = savedProfile, screen = AppScreen.Menu) }
    }

    fun factoryReset() {
        repo.factoryReset()
        _state.update {
            it.copy(
                profile = UserProfile(first_time_setup = true),
                settings = AppSettings(),
                screen = AppScreen.Setup
            )
        }
    }

    /** Handle a normalised navigation action while on the XMB menu. */
    fun onMenuAction(action: GamepadAction) {
        if (_state.value.quickMenuVisible) {
            onQuickMenuAction(action)
            return
        }
        val s = _state.value
        when (action) {
            GamepadAction.LEFT -> moveCategory(-1)
            GamepadAction.RIGHT -> moveCategory(1)
            GamepadAction.UP -> moveItem(-1)
            GamepadAction.DOWN -> moveItem(1)
            GamepadAction.CONFIRM -> s.currentItem?.let { confirmItem(it) }
            GamepadAction.MENU -> openQuickMenu()
            GamepadAction.CANCEL, GamepadAction.NONE -> Unit
        }
    }

    // ---- Quick menu overlay (Android port of QuickMenu.cpp) ----

    fun openQuickMenu() {
        sounds.playOption()
        _state.update { it.copy(quickMenuVisible = true, quickMenuIndex = 0) }
    }

    fun closeQuickMenu() {
        sounds.playCancel()
        _state.update { it.copy(quickMenuVisible = false) }
    }

    private fun onQuickMenuAction(action: GamepadAction) {
        when (action) {
            GamepadAction.UP -> moveQuickMenu(-1)
            GamepadAction.DOWN -> moveQuickMenu(1)
            GamepadAction.CONFIRM -> confirmQuickMenu()
            GamepadAction.CANCEL, GamepadAction.MENU -> closeQuickMenu()
            GamepadAction.LEFT, GamepadAction.RIGHT, GamepadAction.NONE -> Unit
        }
    }

    private fun moveQuickMenu(delta: Int) {
        val before = _state.value.quickMenuIndex
        val last = QuickMenuOption.entries.lastIndex
        _state.update { it.copy(quickMenuIndex = (it.quickMenuIndex + delta).coerceIn(0, last)) }
        if (_state.value.quickMenuIndex != before) sounds.playCursor()
    }

    /** Used by touch: highlight a quick-menu row before confirming it. */
    fun selectQuickMenu(index: Int) = _state.update { it.copy(quickMenuIndex = index) }

    /** Confirm the highlighted quick-menu option. */
    fun confirmQuickMenu() {
        val option = QuickMenuOption.entries[_state.value.quickMenuIndex]
        sounds.playDecide()
        _state.update { it.copy(quickMenuVisible = false) }
        when (option) {
            QuickMenuOption.Resume -> Unit
            QuickMenuOption.ChangeTheme -> goTo(AppScreen.ThemeSelect)
            QuickMenuOption.About -> goTo(AppScreen.About)
            QuickMenuOption.Exit -> _events.tryEmit(UiEvent.Exit)
        }
    }

    private fun moveCategory(delta: Int) {
        val before = _state.value.categoryIndex
        _state.update {
            if (it.categories.isEmpty()) return@update it
            val newIndex = (it.categoryIndex + delta).coerceIn(0, it.categories.lastIndex)
            it.copy(categoryIndex = newIndex, itemIndex = 0)
        }
        if (_state.value.categoryIndex != before) sounds.playCategoryDecide()
    }

    private fun moveItem(delta: Int) {
        val before = _state.value.itemIndex
        _state.update {
            val count = it.currentCategory?.items?.size ?: 0
            if (count == 0) return@update it
            it.copy(itemIndex = (it.itemIndex + delta).coerceIn(0, count - 1))
        }
        if (_state.value.itemIndex != before) sounds.playCursor()
    }

    private fun confirmItem(item: MenuItem) {
        sounds.playDecide()
        when (item.type) {
            "psp_iso", "psp_eboot" -> {
                // Show the PSP boot animation, then launch on its completion.
                _state.update { it.copy(pendingLaunch = item, screen = AppScreen.GameStartup) }
            }
            "theme_select" -> goTo(AppScreen.ThemeSelect)
            "theme_create" -> goTo(AppScreen.ThemeCreate)
            "scan_roms" -> _events.tryEmit(UiEvent.PickRomFolder)
            "how_to_games" -> goTo(AppScreen.HowTo)
            "factory_reset" -> factoryReset()
            "about" -> goTo(AppScreen.About)
            else -> launcher.launch(item, _state.value.settings)
        }
    }

    /**
     * Called by the activity once the user picks a ROM folder. Persists the tree URI,
     * scans it off the main thread, and merges the discovered games into the "games"
     * category so they appear on the XMB.
     */
    fun onRomFolderPicked(treeUri: Uri) {
        val saved = _state.value.settings.copy(games_tree_uri = treeUri.toString())
        repo.saveSettings(saved)
        _state.update { it.copy(settings = saved) }
        viewModelScope.launch {
            val games = withContext(Dispatchers.IO) {
                RomScanner.scan(getApplication(), treeUri)
            }
            if (games.isEmpty()) {
                sounds.playError()
                return@launch
            }
            sounds.playSystemOk()
            _state.update { st ->
                val updated = st.categories.map { cat ->
                    if (cat.id == "games") cat.copy(items = games) else cat
                }
                st.copy(categories = updated)
            }
        }
    }

    /** Called when the boot animation finishes to actually hand off to PPSSPP. */
    fun onGameStartupFinished() {
        val s = _state.value
        val item = s.pendingLaunch
        if (item != null) launcher.launch(item, s.settings)
        _state.update { it.copy(pendingLaunch = null, screen = AppScreen.Menu) }
    }

    /** Back / cancel from a sub-screen to the XMB menu, with the PSP cancel sound. */
    fun onCancel(target: AppScreen = AppScreen.Menu) {
        sounds.playCancel()
        goTo(target)
    }

    fun isPpssppInstalled(): Boolean = launcher.isPpssppInstalled(_state.value.settings)

    override fun onCleared() {
        sounds.release()
        super.onCleared()
    }
}
