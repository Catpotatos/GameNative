package app.gamenative.ui.components

import android.Manifest
import android.content.Context
import android.net.Uri
import android.os.Build
import android.os.Environment
import android.provider.DocumentsContract
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.compose.ManagedActivityResultLauncher
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.platform.LocalContext
import app.gamenative.R
import app.gamenative.utils.CustomGameScanner
import timber.log.Timber

/**
 * Converts a document tree URI to a file path.
 * Returns null if conversion fails.
 */
fun getPathFromTreeUri(uri: Uri?): String? {
    if (uri == null) return null

    return try {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            val docId = DocumentsContract.getTreeDocumentId(uri)

            if (docId.startsWith("primary:")) {
                val path = docId.substringAfter(":")
                val externalStorage = Environment.getExternalStorageDirectory()
                return if (path.isEmpty()) {
                    externalStorage.path
                } else {
                    "${externalStorage.path}/$path"
                }
            }

            if (docId.contains(":")) {
                val parts = docId.split(":", limit = 2)
                if (parts.size == 2) {
                    val volumeId = parts[0]
                    val path = parts[1]
                    val possiblePath = if (path.isEmpty()) {
                        "/storage/$volumeId"
                    } else {
                        "/storage/$volumeId/$path"
                    }
                    val file = java.io.File(possiblePath)
                    if (file.exists() || file.parentFile?.exists() == true) {
                        return possiblePath
                    }

                    // --- START PATCH (API 29 enhanced /storage mapping) ---
                    // Reason: On some Android 10 devices the naive mapping "/storage/<volumeId>/<path>"
                    // does not correspond to a real readable directory (vendor-specific docIds like
                    // "msd:45512" or UUID-like ids). To avoid rejecting valid selections we try a
                    // small set of heuristics under /storage to find a candidate that exists.
                    // This block is intentionally guarded to only run on Android 10 (API 29).
                    if (Build.VERSION.SDK_INT == Build.VERSION_CODES.Q) {
                        try {
                            val candidates = mutableListOf<String>()
                            // keep the straight guess first
                            candidates.add(possiblePath)

                            // try flipping pieces: sometimes docId is "msd:45512" where the actual
                            // mount point is /storage/45512
                            if (path.isNotEmpty()) candidates.add("/storage/$path")

                            // try the volumeId as a top-level dir
                            candidates.add("/storage/$volumeId")

                            // inspect /storage children and add plausible candidates
                            val storageRoot = java.io.File("/storage")
                            storageRoot.listFiles()?.forEach { child ->
                                val name = child.name
                                if (name.equals(volumeId, ignoreCase = true) ||
                                    name.equals(path, ignoreCase = true) ||
                                    name.contains(volumeId) ||
                                    name.contains(path)) {
                                    val cand = if (path.isEmpty()) {
                                        "/storage/$name"
                                    } else {
                                        "/storage/$name/$path"
                                    }
                                    candidates.add(cand)
                                }
                            }

                            // return first existing candidate
                            for (cand in candidates) {
                                try {
                                    val f = java.io.File(cand)
                                    if (f.exists() && f.isDirectory) {
                                        Timber.tag("CustomGameFolderPicker").d(
                                            "Mapped docId=%s -> %s (via candidates) for uri=%s",
                                            docId,
                                            cand,
                                            uri,
                                        )
                                        return cand
                                    }
                                } catch (_: Exception) {
                                    // ignore candidate errors
                                }
                            }
                        } catch (e: Exception) {
                            Timber.tag("CustomGameFolderPicker").w(e, "Enhanced mapping failed for docId=%s uri=%s", docId, uri)
                        }
                    }
                    // --- END PATCH (API 29 enhanced /storage mapping) ---

                    // fallback to the original guess if nothing matched
                    return possiblePath
                }
            }

            if (!docId.contains(":")) {
                return docId
            }
        }

        uri.path?.let { path ->
            if (path.startsWith("/tree/")) {
                val docId = path.substringAfter("/tree/")
                if (docId.startsWith("primary:")) {
                    val filePath = docId.substringAfter(":")
                    val externalStorage = Environment.getExternalStorageDirectory()
                    return if (filePath.isEmpty()) {
                        externalStorage.path
                    } else {
                        "${externalStorage.path}/$filePath"
                    }
                }
            }
            path
        }
    } catch (e: Exception) {
        Timber.tag("CustomGameFolderPicker").w(e, "getPathFromTreeUri failed for uri=%s", uri)
        null
    }
}

/**
 * Ensures we have the correct permissions for the provided path.
 */
fun requestPermissionsForPath(
    context: Context,
    path: String,
    storagePermissionLauncher: ManagedActivityResultLauncher<Array<String>, Map<String, Boolean>>?,
) {
    val isOutsideSandbox = !path.contains("/Android/data/${context.packageName}") &&
            !path.contains(context.dataDir.path)

    if (!isOutsideSandbox) {
        return
    }

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
        CustomGameScanner.requestManageExternalStoragePermission(context)
    } else {
        val permissions = arrayOf(
            Manifest.permission.READ_EXTERNAL_STORAGE,
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
        )
        storagePermissionLauncher?.launch(permissions)
    }
}

data class CustomGameFolderPicker(
    val launchPicker: () -> Unit,
)

/**
 * Helper for remembering a folder picker launcher that returns a resolved file path.
 */
@Composable
fun rememberCustomGameFolderPicker(
    onPathSelected: (String) -> Unit,
    onFailure: (String) -> Unit = {},
    onCancel: () -> Unit = {},
): CustomGameFolderPicker {
    val context = LocalContext.current
    val pickerLauncher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.OpenDocumentTree(),
    ) { uri: Uri? ->
        if (uri == null) {
            onCancel()
            return@rememberLauncherForActivityResult
        }

        val path = getPathFromTreeUri(uri)
        if (path != null) {
            onPathSelected(path)
        } else {
            onFailure(context.getString(R.string.custom_game_folder_picker_error))
        }
    }

    return remember {
        CustomGameFolderPicker(
            launchPicker = { pickerLauncher.launch(null) },
        )
    }
}
