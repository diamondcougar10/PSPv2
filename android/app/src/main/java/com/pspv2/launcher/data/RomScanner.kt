package com.pspv2.launcher.data

import android.content.Context
import android.net.Uri
import android.provider.DocumentsContract
import android.util.Log

/**
 * Scans a Storage Access Framework tree (a folder the user granted access to) for
 * PSP ROMs, the Android equivalent of the desktop's `RomAssetManager` filesystem
 * walk. Android apps can't freely read arbitrary paths on modern OS versions, so
 * the user picks a folder via ACTION_OPEN_DOCUMENT_TREE and we enumerate it through
 * [DocumentsContract] rather than java.io.File.
 */
object RomScanner {

    private const val TAG = "RomScanner"

    /** ROM file extensions PPSSPP can open. */
    private val ROM_EXTENSIONS = setOf("iso", "cso", "pbp", "chd", "prx", "elf")

    /**
     * Recursively enumerates [treeUri] and returns a [MenuItem] for every ROM found.
     * Each item's [MenuItem.path] is a persistable `content://` document URI that
     * [com.pspv2.launcher.launch.GameLauncher] can hand straight to PPSSPP.
     */
    fun scan(context: Context, treeUri: Uri): List<MenuItem> {
        val resolver = context.contentResolver
        val results = mutableListOf<MenuItem>()
        val rootDocId = DocumentsContract.getTreeDocumentId(treeUri)
        // Iterative DFS to avoid deep recursion on large libraries.
        val stack = ArrayDeque<String>()
        stack.addLast(rootDocId)

        while (stack.isNotEmpty()) {
            val parentDocId = stack.removeLast()
            val childrenUri = DocumentsContract.buildChildDocumentsUriUsingTree(treeUri, parentDocId)
            runCatching {
                resolver.query(
                    childrenUri,
                    arrayOf(
                        DocumentsContract.Document.COLUMN_DOCUMENT_ID,
                        DocumentsContract.Document.COLUMN_DISPLAY_NAME,
                        DocumentsContract.Document.COLUMN_MIME_TYPE
                    ),
                    null, null, null
                )?.use { cursor ->
                    val idCol = cursor.getColumnIndexOrThrow(DocumentsContract.Document.COLUMN_DOCUMENT_ID)
                    val nameCol = cursor.getColumnIndexOrThrow(DocumentsContract.Document.COLUMN_DISPLAY_NAME)
                    val mimeCol = cursor.getColumnIndexOrThrow(DocumentsContract.Document.COLUMN_MIME_TYPE)
                    while (cursor.moveToNext()) {
                        val docId = cursor.getString(idCol)
                        val name = cursor.getString(nameCol) ?: continue
                        val mime = cursor.getString(mimeCol)
                        if (mime == DocumentsContract.Document.MIME_TYPE_DIR) {
                            stack.addLast(docId)
                        } else if (name.substringAfterLast('.', "").lowercase() in ROM_EXTENSIONS) {
                            val docUri = DocumentsContract.buildDocumentUriUsingTree(treeUri, docId)
                            results.add(
                                MenuItem(
                                    label = name.substringBeforeLast('.'),
                                    path = docUri.toString(),
                                    type = "psp_iso",
                                    iconFilename = "psp game.png"
                                )
                            )
                        }
                    }
                }
            }.onFailure { Log.w(TAG, "Failed to list $parentDocId", it) }
        }

        return results.sortedBy { it.label.lowercase() }
    }
}
