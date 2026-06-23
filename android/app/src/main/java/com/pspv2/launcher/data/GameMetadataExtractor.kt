package com.pspv2.launcher.data

import android.content.Context
import android.net.Uri
import android.util.Log
import java.io.Closeable
import java.io.FileInputStream
import java.io.RandomAccessFile
import java.nio.ByteBuffer
import java.nio.channels.FileChannel

/**
 * Metadata pulled out of a PSP ROM, mirroring the desktop GameMetadataExtractor.
 * `iconData` = ICON0.PNG, `backgroundData` = PIC1.PNG, `soundData` = SND0.AT3.
 */
data class GameMetadata(
    val title: String = "",
    val gameId: String = "",
    val iconData: ByteArray? = null,
    val backgroundData: ByteArray? = null,
    val soundData: ByteArray? = null
)

/**
 * Kotlin port of the desktop `GameMetadataExtractor`: reads the embedded icon,
 * background, title and intro-audio out of a PSP ROM without any external tools.
 *
 * Handles both container formats:
 *  - PBP (EBOOT): a fixed header of section offsets (PARAM.SFO, ICON0, PIC1, SND0…).
 *  - ISO 9660 (UMD image): a minimal directory walk to PSP_GAME/ to find the files.
 *
 * Works against both app-private files (absolute paths) and SAF `content://` ROMs
 * by opening a seekable [FileChannel] over either source.
 */
object GameMetadataExtractor {

    private const val TAG = "GameMetadata"
    private const val SECTOR = 2048L

    /** Reads metadata from a ROM identified by an absolute path or a content:// URI. */
    fun extract(context: Context, source: String): GameMetadata? {
        return openSource(context, source)?.use { src -> extractFromChannel(src.channel) }
    }

    /** Pairs a [FileChannel] with the descriptor that must outlive it. */
    private class Source(val channel: FileChannel, private val extra: Closeable?) : Closeable {
        override fun close() {
            runCatching { channel.close() }
            runCatching { extra?.close() }
        }
    }

    private fun openSource(context: Context, source: String): Source? = runCatching {
        if (source.startsWith("content://")) {
            val pfd = context.contentResolver.openFileDescriptor(Uri.parse(source), "r")
                ?: return null
            val fis = FileInputStream(pfd.fileDescriptor)
            Source(fis.channel, pfd)
        } else {
            val raf = RandomAccessFile(source, "r")
            Source(raf.channel, raf)
        }
    }.getOrElse {
        Log.w(TAG, "Could not open ROM: $source", it)
        null
    }

    private fun extractFromChannel(ch: FileChannel): GameMetadata? {
        val magic = ch.readAt(0, 4) ?: return null
        return if (magic.size == 4 && magic[0].toInt() == 0 &&
            magic[1] == 'P'.code.toByte() && magic[2] == 'B'.code.toByte() &&
            magic[3] == 'P'.code.toByte()
        ) {
            extractFromPbp(ch)
        } else {
            extractFromIso(ch)
        }
    }

    // --- PBP ---------------------------------------------------------------

    private fun extractFromPbp(ch: FileChannel): GameMetadata? {
        // Header: magic[4] version[4] then 8 LE offsets.
        val header = ch.readAt(0, 40) ?: return null
        if (header.size < 40) return null
        val offParamSfo = header.le32(0x08)
        val offIcon0 = header.le32(0x0C)
        val offIcon1 = header.le32(0x10)
        val offPic1 = header.le32(0x18)
        val offSnd0 = header.le32(0x1C)
        val offDataPsp = header.le32(0x20)

        val sfo = ch.readBetween(offParamSfo, offIcon0)
        val icon = ch.readBetween(offIcon0, offIcon1)
        val pic1 = ch.readBetween(offPic1, offSnd0)
        val snd0 = ch.readBetween(offSnd0, offDataPsp)

        return GameMetadata(
            title = sfo?.let { parseSfoString(it, "TITLE") }.orEmpty(),
            gameId = sfo?.let { parseSfoString(it, "DISC_ID") }.orEmpty(),
            iconData = icon,
            backgroundData = pic1,
            soundData = snd0
        )
    }

    // --- ISO 9660 ----------------------------------------------------------

    private data class IsoDirEntry(
        val lba: Long,
        val size: Long,
        val isDirectory: Boolean,
        val name: String
    )

    private fun extractFromIso(ch: FileChannel): GameMetadata? {
        val pvd = ch.readAt(16 * SECTOR, 2048) ?: return null
        if (pvd.size < 162) return null
        // "CD001" magic at offset 1.
        if (pvd[1] != 'C'.code.toByte() || pvd[2] != 'D'.code.toByte() ||
            pvd[3] != '0'.code.toByte() || pvd[4] != '0'.code.toByte() ||
            pvd[5] != '1'.code.toByte()
        ) return null

        val rootLba = pvd.le32(156 + 2)
        val rootSize = pvd.le32(156 + 10)
        val rootEntries = readIsoDirectory(ch, rootLba, rootSize)

        val pspGame = rootEntries.firstOrNull { it.isDirectory && it.name == "PSP_GAME" }
            ?: return null
        val gameEntries = readIsoDirectory(ch, pspGame.lba, pspGame.size)

        var title = ""
        var gameId = ""
        var icon: ByteArray? = null
        var background: ByteArray? = null
        var sound: ByteArray? = null
        for (entry in gameEntries) {
            if (entry.isDirectory) continue
            when (entry.name) {
                "PARAM.SFO" -> ch.readAt(entry.lba * SECTOR, entry.size.toInt())?.let {
                    title = parseSfoString(it, "TITLE")
                    gameId = parseSfoString(it, "DISC_ID")
                }
                "ICON0.PNG" -> icon = ch.readAt(entry.lba * SECTOR, entry.size.toInt())
                "PIC1.PNG" -> background = ch.readAt(entry.lba * SECTOR, entry.size.toInt())
                "SND0.AT3" -> sound = ch.readAt(entry.lba * SECTOR, entry.size.toInt())
            }
        }
        return GameMetadata(title, gameId, icon, background, sound)
    }

    private fun readIsoDirectory(ch: FileChannel, lba: Long, size: Long): List<IsoDirEntry> {
        val buffer = ch.readAt(lba * SECTOR, size.toInt()) ?: return emptyList()
        val entries = mutableListOf<IsoDirEntry>()
        var offset = 0
        while (offset < buffer.size) {
            val len = buffer[offset].toInt() and 0xFF
            if (len == 0) {
                // Records never cross a sector; skip the padding to the next sector.
                val inSector = offset % SECTOR.toInt()
                offset += SECTOR.toInt() - inSector
                continue
            }
            if (offset + 33 > buffer.size) break
            val extent = buffer.le32(offset + 2)
            val dataLen = buffer.le32(offset + 10)
            val flags = buffer[offset + 25].toInt()
            val nameLen = buffer[offset + 33 - 1].toInt() and 0xFF
            val name = when {
                nameLen == 1 && buffer[offset + 33].toInt() == 0 -> "."
                nameLen == 1 && buffer[offset + 33].toInt() == 1 -> ".."
                offset + 33 + nameLen <= buffer.size ->
                    String(buffer, offset + 33, nameLen, Charsets.US_ASCII).substringBefore(';')
                else -> ""
            }
            entries.add(IsoDirEntry(extent, dataLen, (flags and 2) != 0, name))
            offset += len
        }
        return entries
    }

    // --- SFO ---------------------------------------------------------------

    private fun parseSfoString(sfo: ByteArray, key: String): String {
        if (sfo.size < 20) return ""
        // Magic "\0PSF".
        if (sfo[0].toInt() != 0 || sfo[1] != 'P'.code.toByte() ||
            sfo[2] != 'S'.code.toByte() || sfo[3] != 'F'.code.toByte()
        ) return ""

        val keyTableOffset = sfo.le32(0x08).toInt()
        val dataTableOffset = sfo.le32(0x0C).toInt()
        val entries = sfo.le32(0x10).toInt()
        for (i in 0 until entries) {
            val entryOffset = 0x14 + i * 16
            if (entryOffset + 16 > sfo.size) break
            val keyOffset = sfo.le16(entryOffset)
            val dataLen = sfo.le32(entryOffset + 4).toInt()
            val dataOffset = sfo.le32(entryOffset + 12).toInt()
            val keyStart = keyTableOffset + keyOffset
            if (keyStart >= sfo.size) continue
            val currentKey = cString(sfo, keyStart)
            if (currentKey == key) {
                val dataStart = dataTableOffset + dataOffset
                if (dataStart + dataLen > sfo.size) return ""
                return String(sfo, dataStart, dataLen, Charsets.UTF_8).trim('\u0000').trim()
            }
        }
        return ""
    }

    private fun cString(data: ByteArray, start: Int): String {
        var end = start
        while (end < data.size && data[end].toInt() != 0) end++
        return String(data, start, end - start, Charsets.US_ASCII)
    }

    // --- channel helpers ---------------------------------------------------

    /** Reads exactly up to [len] bytes from absolute position [pos]; null on failure. */
    private fun FileChannel.readAt(pos: Long, len: Int): ByteArray? {
        if (len <= 0) return ByteArray(0)
        return runCatching {
            val buf = ByteBuffer.allocate(len)
            position(pos)
            var read = 0
            while (read < len) {
                val n = read(buf)
                if (n < 0) break
                read += n
            }
            if (read == len) buf.array() else buf.array().copyOf(read)
        }.getOrNull()
    }

    /** Reads the section between two offsets, guarding against bad/zero ranges. */
    private fun FileChannel.readBetween(start: Long, end: Long): ByteArray? {
        if (end <= start) return null
        val len = (end - start)
        if (len <= 0 || len > 64L * 1024 * 1024) return null
        return readAt(start, len.toInt())
    }

    private fun ByteArray.le16(off: Int): Int =
        (this[off].toInt() and 0xFF) or ((this[off + 1].toInt() and 0xFF) shl 8)

    private fun ByteArray.le32(off: Int): Long =
        (this[off].toLong() and 0xFF) or
            ((this[off + 1].toLong() and 0xFF) shl 8) or
            ((this[off + 2].toLong() and 0xFF) shl 16) or
            ((this[off + 3].toLong() and 0xFF) shl 24)
}
