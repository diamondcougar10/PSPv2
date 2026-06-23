package com.pspv2.launcher.data

import android.content.Context
import android.net.Uri
import android.provider.OpenableColumns
import android.util.Log
import com.github.junrar.Archive
import org.apache.commons.compress.archivers.ArchiveEntry
import org.apache.commons.compress.archivers.ArchiveInputStream
import org.apache.commons.compress.archivers.ArchiveStreamFactory
import org.apache.commons.compress.archivers.sevenz.SevenZFile
import org.apache.commons.compress.compressors.CompressorStreamFactory
import java.io.BufferedInputStream
import java.io.File
import java.io.FileInputStream
import java.io.FileOutputStream
import java.io.InputStream

/**
 * Imports ("installs") a downloaded PSP ROM into the app's private games folder so it
 * shows up in the XMB Games category and can be handed to PPSSPP.
 *
 * Format-agnostic: a raw ROM is copied as-is, and any common archive is auto-detected
 * and the first PSP ROM inside is extracted. Supported containers:
 *  - zip, 7z, rar, tar, ar, cpio, arj, dump
 *  - single-stream compressors: gzip (.gz), bzip2 (.bz2), xz (.xz), lzma, and the
 *    common combos like .tar.gz / .tgz (decompressed, then the inner archive is read)
 *
 * Files land in `filesDir/games/`; [com.pspv2.launcher.launch.GameLauncher] exposes
 * them to PPSSPP through a FileProvider content URI.
 */
object RomImporter {

    private const val TAG = "RomImporter"
    private const val BUFFER_SIZE = 64 * 1024

    /** ROM file extensions PPSSPP can open. */
    private val ROM_EXTENSIONS = setOf("iso", "cso", "pbp", "chd", "prx", "elf")

    /** Outcome of an import attempt, surfaced to the UI. */
    data class Result(val item: MenuItem?, val message: String, val success: Boolean)

    /** Internal extraction outcome so we can tell "not this format" from "no ROM inside". */
    private sealed interface Extracted {
        data class Done(val file: File) : Extracted
        object NoRom : Extracted
        object NotThisFormat : Extracted
    }

    /** Private directory that holds imported ROMs. */
    fun gamesDir(context: Context): File =
        File(context.filesDir, "games").apply { mkdirs() }

    /**
     * Imports the file at [source] (a content:// URI from the document picker) into the
     * games folder, reporting human-readable progress through [onProgress].
     */
    fun import(context: Context, source: Uri, onProgress: (String) -> Unit): Result {
        val displayName = queryDisplayName(context, source) ?: "rom_${System.currentTimeMillis()}"
        val ext = displayName.substringAfterLast('.', "").lowercase()
        return try {
            if (ext in ROM_EXTENSIONS) {
                importDirect(context, source, displayName, onProgress)
            } else {
                importArchive(context, source, displayName, onProgress)
            }
        } catch (e: Exception) {
            Log.e(TAG, "Import failed", e)
            Result(null, "Import failed: ${e.message ?: "unknown error"}", false)
        }
    }

    /**
     * Stages the picked file to the cache (so seek-based formats like 7z/rar work) and
     * tries each extraction strategy until one recognises the format.
     */
    private fun importArchive(
        context: Context,
        source: Uri,
        archiveName: String,
        onProgress: (String) -> Unit
    ): Result {
        onProgress("Opening $archiveName…")
        val temp = File.createTempFile("import", ".bin", context.cacheDir)
        try {
            context.contentResolver.openInputStream(source)?.use { raw ->
                FileOutputStream(temp).use { out ->
                    copyStream(raw, out) { bytes -> onProgress("Reading $archiveName (${mb(bytes)})") }
                }
            } ?: return Result(null, "Could not open $archiveName", false)

            val gamesDir = gamesDir(context)
            // Try every container type; the first one that recognises the bytes wins.
            val strategies = listOf(
                { extractSevenZ(temp, gamesDir, onProgress) },
                { extractRar(temp, gamesDir, onProgress) },
                { extractWithArchiveFactory(temp, gamesDir, onProgress) },
                { extractCompressed(temp, archiveName, gamesDir, onProgress) }
            )
            var recognised = false
            for (strategy in strategies) {
                when (val outcome = strategy()) {
                    is Extracted.Done ->
                        return Result(
                            toMenuItem(outcome.file),
                            "Installed ${outcome.file.nameWithoutExtension}",
                            true
                        )
                    Extracted.NoRom -> recognised = true
                    Extracted.NotThisFormat -> Unit
                }
            }
            return if (recognised) {
                Result(null, "No PSP ROM found inside $archiveName", false)
            } else {
                Result(null, "Unsupported or corrupt archive: $archiveName", false)
            }
        } finally {
            temp.delete()
        }
    }

    /** 7z is seek-based, so it reads the staged file directly. */
    private fun extractSevenZ(temp: File, gamesDir: File, onProgress: (String) -> Unit): Extracted {
        return try {
            SevenZFile.builder().setFile(temp).get().use { sevenZ ->
                var entry = sevenZ.nextEntry
                while (entry != null) {
                    val name = entry.name?.substringAfterLast('/').orEmpty()
                    if (!entry.isDirectory && name.romMatches()) {
                        return writeEntry(gamesDir, name, sevenZ.getInputStream(entry), onProgress)
                    }
                    entry = sevenZ.nextEntry
                }
            }
            Extracted.NoRom
        } catch (e: Exception) {
            Extracted.NotThisFormat
        }
    }

    /** RAR (incl. RAR5) via junrar. */
    private fun extractRar(temp: File, gamesDir: File, onProgress: (String) -> Unit): Extracted {
        return try {
            Archive(temp).use { archive ->
                var header = archive.nextFileHeader()
                while (header != null) {
                    val name = (header.fileName ?: "").substringAfterLast('/').substringAfterLast('\\')
                    if (!header.isDirectory && name.romMatches()) {
                        val outFile = File(gamesDir, sanitize(name))
                        onProgress("Extracting $name…")
                        FileOutputStream(outFile).use { out -> archive.extractFile(header, out) }
                        return Extracted.Done(outFile)
                    }
                    header = archive.nextFileHeader()
                }
            }
            Extracted.NoRom
        } catch (e: Exception) {
            Extracted.NotThisFormat
        }
    }

    /** zip, tar, ar, cpio, arj, dump — auto-detected from the stream signature. */
    private fun extractWithArchiveFactory(
        temp: File,
        gamesDir: File,
        onProgress: (String) -> Unit
    ): Extracted {
        return try {
            BufferedInputStream(FileInputStream(temp)).use { bis ->
                ArchiveStreamFactory().createArchiveInputStream<ArchiveEntry>(bis).use { ai ->
                    findRomInArchive(ai, gamesDir, onProgress)
                }
            }
        } catch (e: Exception) {
            Extracted.NotThisFormat
        }
    }

    /**
     * Single-stream compressors (.gz/.bz2/.xz/.lzma). The decompressed payload is either
     * an inner archive (e.g. .tar.gz) or a raw ROM, so we re-detect after decompressing.
     */
    private fun extractCompressed(
        temp: File,
        archiveName: String,
        gamesDir: File,
        onProgress: (String) -> Unit
    ): Extracted {
        return try {
            BufferedInputStream(FileInputStream(temp)).use { bis ->
                CompressorStreamFactory().createCompressorInputStream(bis).use { comp ->
                    val buffered = BufferedInputStream(comp)
                    // Case 1: the decompressed stream is itself an archive (tar.gz, tgz…).
                    val asArchive: ArchiveInputStream<*>? = runCatching {
                        ArchiveStreamFactory().createArchiveInputStream<ArchiveEntry>(buffered)
                    }.getOrNull()
                    if (asArchive != null) {
                        return asArchive.use { findRomInArchive(it, gamesDir, onProgress) }
                    }
                    // Case 2: a single compressed ROM (e.g. game.iso.gz). Strip the
                    // compressor extension to recover the ROM name.
                    val innerName = archiveName.substringBeforeLast('.', archiveName)
                    if (innerName.romMatches()) {
                        return writeEntry(gamesDir, innerName, buffered, onProgress)
                    }
                    Extracted.NoRom
                }
            }
        } catch (e: Exception) {
            Extracted.NotThisFormat
        }
    }

    /** Walks an [ArchiveInputStream], extracting the first PSP ROM entry it finds. */
    private fun findRomInArchive(
        ai: ArchiveInputStream<*>,
        gamesDir: File,
        onProgress: (String) -> Unit
    ): Extracted {
        var entry = ai.nextEntry
        while (entry != null) {
            val name = entry.name.substringAfterLast('/').substringAfterLast('\\')
            if (!entry.isDirectory && name.romMatches() && ai.canReadEntryData(entry)) {
                return writeEntry(gamesDir, name, ai, onProgress)
            }
            entry = ai.nextEntry
        }
        return Extracted.NoRom
    }

    private fun writeEntry(
        gamesDir: File,
        entryName: String,
        input: InputStream,
        onProgress: (String) -> Unit
    ): Extracted {
        val outFile = File(gamesDir, sanitize(entryName))
        onProgress("Extracting $entryName…")
        FileOutputStream(outFile).use { out ->
            copyStream(input, out) { bytes -> onProgress("Extracting $entryName (${mb(bytes)})") }
        }
        return Extracted.Done(outFile)
    }

    private fun importDirect(
        context: Context,
        source: Uri,
        displayName: String,
        onProgress: (String) -> Unit
    ): Result {
        val gamesDir = gamesDir(context)
        val outFile = File(gamesDir, sanitize(displayName))
        onProgress("Copying $displayName…")
        val input = context.contentResolver.openInputStream(source)
            ?: return Result(null, "Could not open $displayName", false)
        input.use { raw ->
            FileOutputStream(outFile).use { out ->
                copyStream(raw, out) { bytes -> onProgress("Copying $displayName (${mb(bytes)})") }
            }
        }
        return Result(toMenuItem(outFile), "Installed ${outFile.nameWithoutExtension}", true)
    }

    private fun toMenuItem(file: File) = MenuItem(
        label = file.nameWithoutExtension,
        path = file.absolutePath,
        type = "psp_iso",
        iconFilename = "psp game.png"
    )

    /** True if this file name ends in a PSP ROM extension. */
    private fun String.romMatches(): Boolean =
        isNotBlank() && substringAfterLast('.', "").lowercase() in ROM_EXTENSIONS

    /** Copies [input] to [output], invoking [report] roughly every few MB. */
    private fun copyStream(input: InputStream, output: FileOutputStream, report: (Long) -> Unit) {
        val buffer = ByteArray(BUFFER_SIZE)
        var total = 0L
        var sinceReport = 0L
        while (true) {
            val read = input.read(buffer)
            if (read < 0) break
            output.write(buffer, 0, read)
            total += read
            sinceReport += read
            if (sinceReport >= 4L * 1024 * 1024) {
                report(total)
                sinceReport = 0
            }
        }
        output.flush()
    }

    private fun mb(bytes: Long) = "%.1f MB".format(bytes / 1_048_576.0)

    /** Strips any path separators so an archive entry can't escape the games folder. */
    private fun sanitize(name: String): String =
        name.substringAfterLast('/').substringAfterLast('\\')

    private fun queryDisplayName(context: Context, uri: Uri): String? {
        if (uri.scheme == "file") return uri.lastPathSegment
        return runCatching {
            context.contentResolver.query(uri, arrayOf(OpenableColumns.DISPLAY_NAME), null, null, null)
                ?.use { cursor ->
                    if (cursor.moveToFirst()) cursor.getString(0) else null
                }
        }.getOrNull()
    }
}
