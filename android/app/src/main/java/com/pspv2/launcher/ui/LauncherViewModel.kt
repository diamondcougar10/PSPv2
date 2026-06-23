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
import com.pspv2.launcher.data.RomImporter
import com.pspv2.launcher.data.RomScanner
import com.pspv2.launcher.data.UserProfile
import com.pspv2.launcher.input.GamepadAction
import com.pspv2.launcher.launch.GameLauncher
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
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
enum class UiEvent { Exit, PickRomFolder, PickRomFile }

/** Options available in the XMB quick menu overlay (Android port of QuickMenu.cpp). */
enum class QuickMenuOption(val label: String) {
    Resume("Resume"),
    ChangeTheme("Change Theme"),
    About("About"),
    Exit("Exit PSPV2")
}

/** Progress / result banner shown while importing a downloaded ROM. */
data class ImportStatus(
    val message: String,
    val busy: Boolean,
    val isError: Boolean = false
)

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
    val customTheme: CustomTheme = CustomTheme(),
    /** True when the user tried to launch a game but no PPSSPP build is installed. */
    val ppssppMissing: Boolean = false,
    /** Non-null while a ROM import is running or its result is being shown. */
    val importStatus: ImportStatus? = null
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

    /**
     * The non-game action items declared in the menu.json "games" category (e.g. the
     * "Install Game" entry). Preserved across rescans/imports so the category always
     * keeps its actions even when no ROMs are present yet.
     */
    private var gamesAnchors: List<MenuItem> = emptyList()

    /** ROMs discovered by scanning the user's chosen SAF folder (Scan ROM Folder). */
    private var safGames: List<MenuItem> = emptyList()

    /** One-shot UI events the host activity reacts to (e.g. exit). */
    private val _events = MutableSharedFlow<UiEvent>(extraBufferCapacity = 1)
    val events = _events.asSharedFlow()

    /**
     * Scroll nudges (in dp) for long-text screens like "How to Add Games", so the
     * gamepad's D-pad / analog stick can scroll content that has no selectable rows.
     */
    private val _scrollNudges = MutableSharedFlow<Int>(extraBufferCapacity = 8)
    val scrollNudges = _scrollNudges.asSharedFlow()

    /** Emit a scroll request consumed by the currently visible scrollable screen. */
    fun nudgeScroll(deltaDp: Int) {
        sounds.playCursor()
        _scrollNudges.tryEmit(deltaDp)
    }

    /** Skip the intro animation when the player presses a button. */
    fun skipIntro() {
        if (_state.value.screen == AppScreen.Intro) onIntroFinished()
    }

    /** Skip the PSP boot animation and hand straight off to PPSSPP. */
    fun skipGameStartup() {
        if (_state.value.screen == AppScreen.GameStartup) onGameStartupFinished()
    }

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
        // Remember the games category's non-ROM action items (e.g. "Install Game") so
        // they survive rescans, then merge any previously imported ROMs into view.
        gamesAnchors = menu.categories.firstOrNull { it.id == "games" }
            ?.items.orEmpty()
            .filter { it.type != "psp_iso" && it.type != "psp_eboot" }
        rebuildGamesCategory()
        // If the user previously chose a ROM folder, rescan it on launch so newly
        // added games show up without re-picking the folder.
        if (settings.games_tree_uri.isNotBlank()) {
            rescanSavedRomFolder(settings.games_tree_uri)
        }
        // Scan for an already-installed PPSSPP build and remember it, so the user
        // never has to configure anything when PPSSPP is already on the device.
        cacheInstalledPpsspp(settings)
    }

    /**
     * Detects an installed PPSSPP build and, if found, stores its package name in
     * settings. Runs once on startup; no-op (and no user action) when PPSSPP is
     * already installed and previously remembered.
     */
    private fun cacheInstalledPpsspp(settings: AppSettings) {
        val detected = launcher.detectInstalledPackage(settings) ?: return
        if (detected != settings.ppsspp_package) {
            val saved = _state.value.settings.copy(ppsspp_package = detected)
            repo.saveSettings(saved)
            _state.update { it.copy(settings = saved) }
        }
    }

    private fun rescanSavedRomFolder(treeUriString: String) {
        val treeUri = runCatching { treeUriString.toUri() }.getOrNull() ?: return
        viewModelScope.launch {
            val games = withContext(Dispatchers.IO) {
                runCatching { RomScanner.scan(getApplication(), treeUri) }.getOrDefault(emptyList())
            }
            if (games.isNotEmpty()) {
                safGames = games
                rebuildGamesCategory()
            }
        }
    }

    /**
     * Recomputes the "games" category items from all sources: the permanent action
     * anchors (e.g. "Install Game"), imported ROMs on disk, and SAF-scanned ROMs.
     * De-duplicated by path so a game listed in two places only appears once.
     */
    private fun rebuildGamesCategory() {
        val imported = runCatching { repo.scanImportedGames() }.getOrDefault(emptyList())
        val games = (imported + safGames).distinctBy { it.path }.sortedBy { it.label.lowercase() }
        val items = gamesAnchors + games
        _state.update { st ->
            val updated = st.categories.map { cat ->
                if (cat.id == "games") cat.copy(items = items) else cat
            }
            st.copy(categories = updated)
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
                // Only show the boot animation if PPSSPP is actually present; otherwise
                // prompt the user to install it instead of booting into nothing.
                if (launcher.isPpssppInstalled(_state.value.settings)) {
                    _state.update { it.copy(pendingLaunch = item, screen = AppScreen.GameStartup) }
                } else {
                    sounds.playError()
                    _state.update { it.copy(ppssppMissing = true) }
                }
            }
            "theme_select" -> goTo(AppScreen.ThemeSelect)
            "theme_create" -> goTo(AppScreen.ThemeCreate)
            "scan_roms" -> _events.tryEmit(UiEvent.PickRomFolder)
            "import_rom" -> _events.tryEmit(UiEvent.PickRomFile)
            "how_to_games" -> goTo(AppScreen.HowTo)
            "factory_reset" -> factoryReset()
            "about" -> goTo(AppScreen.About)
            "exit_app" -> _events.tryEmit(UiEvent.Exit)
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
            safGames = games
            rebuildGamesCategory()
        }
    }

    /**
     * Called by the activity once the user picks a downloaded ROM/.zip from the
     * document picker. Copies/extracts it into the games folder off the main thread,
     * streaming progress to the on-screen banner, then refreshes the Games category.
     */
    fun onRomFilePicked(source: Uri) {
        _state.update { it.copy(importStatus = ImportStatus("Starting import…", busy = true)) }
        sounds.playDecide()
        viewModelScope.launch {
            val result = withContext(Dispatchers.IO) {
                RomImporter.import(getApplication(), source) { progress ->
                    _state.update { it.copy(importStatus = ImportStatus(progress, busy = true)) }
                }
            }
            if (result.success) {
                sounds.playSystemOk()
                rebuildGamesCategory()
                focusGamesCategory()
            } else {
                sounds.playError()
            }
            _state.update {
                it.copy(importStatus = ImportStatus(result.message, busy = false, isError = !result.success))
            }
            // Auto-dismiss the banner after letting the user read the result.
            delay(if (result.success) 3500 else 5000)
            _state.update {
                if (it.importStatus?.busy == true) it else it.copy(importStatus = null)
            }
        }
    }

    /** Manually dismiss the import banner (e.g. on a button press). */
    fun dismissImportStatus() {
        _state.update { it.copy(importStatus = null) }
    }

    /** Moves the XMB selection to the newly populated Games category. */
    private fun focusGamesCategory() {
        val idx = _state.value.categories.indexOfFirst { it.id == "games" }
        if (idx >= 0) _state.update { it.copy(categoryIndex = idx, itemIndex = 0) }
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

    /** Dismisses the "PPSSPP not installed" prompt without taking action. */
    fun dismissPpssppPrompt() {
        sounds.playCancel()
        _state.update { it.copy(ppssppMissing = false) }
    }

    /** Opens the PPSSPP store listing, then dismisses the prompt. */
    fun installPpsspp() {
        sounds.playSystemOk()
        launcher.openPpssppStore()
        _state.update { it.copy(ppssppMissing = false) }
    }

    override fun onCleared() {
        sounds.release()
        super.onCleared()
    }
}
