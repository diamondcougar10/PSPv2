package com.pspv2.launcher.audio

import android.content.Context
import android.media.AudioAttributes
import android.media.SoundPool
import android.util.Log

/**
 * PSP-style UI sound bank, the Android port of [src/UiSoundBank.cpp]. Uses
 * [SoundPool] (designed for short, low-latency UI feedback) instead of SFML's
 * sf::Sound. All clips are loaded from the bundled `assets/Sounds/` folder.
 *
 * As on the desktop, both firmware 1.00 and 1.50 cursor / "system ok" variants are
 * loaded; [useV150] picks which one to play for those two effects.
 */
class UiSoundBank(private val context: Context) {

    private val pool: SoundPool = SoundPool.Builder()
        .setMaxStreams(4)
        .setAudioAttributes(
            AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_GAME)
                .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                .build()
        )
        .build()

    /** sample name -> SoundPool sample id. */
    private val sampleIds = HashMap<String, Int>()
    /** sample ids that have finished loading and are safe to play. */
    private val ready = HashSet<Int>()

    /** Prefer the firmware 1.50 cursor / system-ok variants when true. */
    var useV150: Boolean = true
    /** Master enable; when false [play] is a no-op. */
    var enabled: Boolean = true

    init {
        pool.setOnLoadCompleteListener { _, sampleId, status ->
            if (status == 0) ready.add(sampleId)
        }
        load()
    }

    private fun load() {
        // Firmware 1.50 variants.
        register(OPENING, "PSP-1.50_opening_plugin.rco-snd_opening.mp3")
        register(CURSOR_150, "PSP-1.50_system_plugin.rco-snd_cursor.mp3")
        register(SYSTEM_OK_150, "PSP-1.50_system_plugin.rco-snd_system_ok.mp3")
        // Firmware 1.00 variants.
        register(CURSOR_100, "PSP-1.00_system_plugin.rco-snd_cursor.mp3")
        register(SYSTEM_OK_100, "PSP-1.00_system_plugin.rco-snd_system_ok.mp3")
        register(CANCEL, "PSP-1.00_system_plugin.rco-snd_cancel.mp3")
        register(CATEGORY_DECIDE, "PSP-1.00_system_plugin.rco-snd_category_decide.mp3")
        register(DECIDE, "PSP-1.00_system_plugin.rco-snd_decide.mp3")
        register(OPTION, "PSP-1.00_system_plugin.rco-snd_option.mp3")
        register(ERROR, "PSP-1.00_system_plugin.rco-snd_error.mp3")
    }

    private fun register(name: String, filename: String) {
        runCatching {
            context.assets.openFd("Sounds/$filename").use { afd ->
                sampleIds[name] = pool.load(afd, 1)
            }
        }.onFailure {
            Log.w(TAG, "Failed to load UI sound: $filename", it)
        }
    }

    private fun play(name: String) {
        if (!enabled) return
        val id = sampleIds[name] ?: return
        if (id !in ready) return
        pool.play(id, 1f, 1f, 1, 0, 1f)
    }

    fun playOpening() = play(OPENING)
    fun playCursor() = play(if (useV150) CURSOR_150 else CURSOR_100)
    fun playCategoryDecide() = play(CATEGORY_DECIDE)
    fun playDecide() = play(DECIDE)
    fun playCancel() = play(CANCEL)
    fun playOption() = play(OPTION)
    fun playSystemOk() = play(if (useV150) SYSTEM_OK_150 else SYSTEM_OK_100)
    fun playError() = play(ERROR)

    /** Free native resources. Call from the owner's onCleared/onDestroy. */
    fun release() {
        pool.release()
        sampleIds.clear()
        ready.clear()
    }

    private companion object {
        const val TAG = "UiSoundBank"
        const val OPENING = "opening"
        const val CURSOR_150 = "cursor150"
        const val CURSOR_100 = "cursor100"
        const val SYSTEM_OK_150 = "systemOk150"
        const val SYSTEM_OK_100 = "systemOk100"
        const val CANCEL = "cancel"
        const val CATEGORY_DECIDE = "categoryDecide"
        const val DECIDE = "decide"
        const val OPTION = "option"
        const val ERROR = "error"
    }
}
