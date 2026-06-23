package com.pspv2.launcher.launch

import android.content.ActivityNotFoundException
import android.content.Context
import android.content.Intent
import android.net.Uri
import android.os.Build
import android.provider.Settings
import android.widget.Toast
import androidx.core.content.FileProvider
import androidx.core.net.toUri
import com.pspv2.launcher.data.AppSettings
import com.pspv2.launcher.data.MenuItem

/**
 * Android replacement for the desktop Launcher.cpp. Instead of spawning a
 * PPSSPP.exe process with `cmd /c start`, it hands the selected game to the
 * installed PPSSPP Android app through an Intent, and routes the other XMB item
 * types to the appropriate Android system handlers.
 */
class GameLauncher(private val context: Context) {

    enum class Result { LAUNCHED, PPSSPP_NOT_INSTALLED, INVALID_PATH, HANDLED, UNSUPPORTED }

    fun launch(item: MenuItem, settings: AppSettings): Result {
        return when (item.type) {
            "psp_iso", "psp_eboot" -> launchPsp(item, settings)
            "web_url" -> openUri(item.path)
            "app_gallery" -> openAppCategory(Intent.CATEGORY_APP_GALLERY, "gallery")
            "app_music" -> openAppCategory(Intent.CATEGORY_APP_MUSIC, "music player")
            "app_calculator" -> openAppCategory(Intent.CATEGORY_APP_CALCULATOR, "calculator")
            "app_files", "folder" -> openFiles()
            "android_settings" -> openSettings()
            "pc_app" -> Result.UNSUPPORTED // desktop-only items have no Android meaning
            else -> Result.UNSUPPORTED
        }
    }

    /** True if a PPSSPP build is installed and can receive the game. */
    fun isPpssppInstalled(settings: AppSettings): Boolean {
        return resolvePpssppPackage(settings) != null
    }

    /**
     * Returns the package name of the installed PPSSPP build, or null if none is
     * present. Exposed so the ViewModel can scan once on startup and remember the
     * result, meaning the user never has to point us at PPSSPP manually when it is
     * already installed.
     */
    fun detectInstalledPackage(settings: AppSettings): String? = resolvePpssppPackage(settings)

    /** Opens the PPSSPP store listing so the user can install it in one tap. */
    fun openPpssppStore(): Result {
        val market = "market://details?id=org.ppsspp.ppsspp".toUri()
        val web = "https://play.google.com/store/apps/details?id=org.ppsspp.ppsspp".toUri()
        val tryOpen = { uri: Uri ->
            runCatching {
                context.startActivity(
                    Intent(Intent.ACTION_VIEW, uri).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                )
                true
            }.getOrDefault(false)
        }
        return if (tryOpen(market) || tryOpen(web)) Result.HANDLED else Result.UNSUPPORTED
    }

    private fun launchPsp(item: MenuItem, settings: AppSettings): Result {
        if (item.path.isBlank()) return Result.INVALID_PATH
        val pkg = resolvePpssppPackage(settings) ?: return Result.PPSSPP_NOT_INSTALLED

        val gameUri = toGameUri(item.path) ?: return Result.INVALID_PATH

        // PPSSPP's main activity accepts an ACTION_VIEW intent pointing at the ROM.
        val intent = Intent(Intent.ACTION_VIEW).apply {
            setDataAndType(gameUri, "*/*")
            setPackage(pkg)
            addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
        }
        return try {
            context.startActivity(intent)
            Result.LAUNCHED
        } catch (e: ActivityNotFoundException) {
            Result.PPSSPP_NOT_INSTALLED
        }
    }

    private fun toGameUri(path: String): Uri? = runCatching {
        when {
            // Already a usable URI (SAF document picked by the user, etc.).
            path.startsWith("content://") || path.startsWith("file://") -> path.toUri()
            // A raw file path (an imported ROM in our private games folder). Other apps
            // can't read our files directly, so expose it through the FileProvider.
            else -> FileProvider.getUriForFile(
                context,
                "${context.packageName}.fileprovider",
                java.io.File(path)
            )
        }
    }.getOrNull()

    private fun openUri(url: String): Result {
        val intent = Intent(Intent.ACTION_VIEW, url.toUri())
            .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
        return try {
            context.startActivity(intent)
            Result.HANDLED
        } catch (e: ActivityNotFoundException) {
            toast("No app available to open $url")
            Result.UNSUPPORTED
        }
    }

    /**
     * Opens the device's default app for a well-known category (gallery, music,
     * calculator) using the same mechanism the Android launcher uses, so the user
     * lands in whatever app they already use for that purpose.
     */
    private fun openAppCategory(category: String, label: String): Result {
        val intent = Intent.makeMainSelectorActivity(Intent.ACTION_MAIN, category)
            .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
        return try {
            context.startActivity(intent)
            Result.HANDLED
        } catch (e: ActivityNotFoundException) {
            toast("No $label app found")
            Result.UNSUPPORTED
        }
    }

    /** Opens the system Files app, falling back to a document picker on older devices. */
    private fun openFiles(): Result {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            val filesApp = Intent.makeMainSelectorActivity(
                Intent.ACTION_MAIN, Intent.CATEGORY_APP_FILES
            ).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
            val opened = runCatching { context.startActivity(filesApp); true }.getOrDefault(false)
            if (opened) return Result.HANDLED
        }
        val picker = Intent(Intent.ACTION_GET_CONTENT).apply {
            type = "*/*"
            addCategory(Intent.CATEGORY_OPENABLE)
            addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
        }
        return try {
            context.startActivity(picker)
            Result.HANDLED
        } catch (e: ActivityNotFoundException) {
            toast("No file manager available")
            Result.UNSUPPORTED
        }
    }

    /** Opens the Android system settings. */
    private fun openSettings(): Result {
        val intent = Intent(Settings.ACTION_SETTINGS).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
        return try {
            context.startActivity(intent)
            Result.HANDLED
        } catch (e: ActivityNotFoundException) {
            toast("Settings unavailable")
            Result.UNSUPPORTED
        }
    }

    /**
     * Scans for an installed PPSSPP build and returns its package name.
     *
     * Checks the user-chosen package first, then the two official builds, then any
     * other installed package whose name looks like a PPSSPP variant. This means the
     * launcher "just works" when PPSSPP is already installed, with no user action.
     */
    private fun resolvePpssppPackage(settings: AppSettings): String? {
        val pm = context.packageManager
        val isInstalled = { pkg: String ->
            runCatching { pm.getLaunchIntentForPackage(pkg) != null }.getOrDefault(false)
        }

        // 1. Fast path: known package names (covers practically every real install).
        val known = buildList {
            if (settings.ppsspp_package.isNotBlank()) add(settings.ppsspp_package)
            add("org.ppsspp.ppsspp")
            add("org.ppsspp.ppssppgold")
        }.distinct()
        known.firstOrNull(isInstalled)?.let { return it }

        // 2. Fallback scan: any visible installed package that looks like PPSSPP.
        return runCatching {
            pm.getInstalledPackages(0)
                .map { it.packageName }
                .firstOrNull { it.contains("ppsspp", ignoreCase = true) && isInstalled(it) }
        }.getOrNull()
    }

    private fun toast(msg: String) {
        Toast.makeText(context, msg, Toast.LENGTH_SHORT).show()
    }
}
