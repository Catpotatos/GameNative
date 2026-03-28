package app.gamenative.ui.component.dialog

import androidx.compose.runtime.MutableIntState
import androidx.compose.runtime.MutableState
import app.gamenative.utils.ContainerUtils
import app.gamenative.utils.ManifestComponentHelper
import app.gamenative.utils.ManifestEntry
import com.winlator.box86_64.Box86_64Preset
import com.winlator.contents.ContentProfile
import com.winlator.container.ContainerData
import com.winlator.core.KeyValueSet
import com.winlator.fexcore.FEXCorePreset

/**
 * Describes a single ANGLE environment variable with its key, default value,
 * and a human-readable description shown in the settings UI.
 * When [readOnly] is true, the value is displayed but not editable (e.g., LD_LIBRARY_PATH).
 */
data class AngleEnvVarDef(
    val key: String,
    val defaultValue: String,
    val description: String,
    val readOnly: Boolean = false,
)

/**
 * All ANGLE environment variables set at runtime by extractGraphicsDriverFiles().
 * These are exposed in the Graphics settings tab when ANGLE is the selected driver,
 * allowing users to inspect and override values for debugging.
 */
val ANGLE_ENV_VAR_DEFS: List<AngleEnvVarDef> = listOf(
    AngleEnvVarDef("ANGLE_DEFAULT_PLATFORM", "vulkan", "ANGLE rendering backend (only 'vulkan' is compiled in)"),
    AngleEnvVarDef("LIBANGLE_DEFAULT_PLATFORM", "vulkan", "Alias of ANGLE_DEFAULT_PLATFORM"),
    AngleEnvVarDef("WINE_D3D_CONFIG", "renderer=gl", "Wine Direct3D renderer config"),
    AngleEnvVarDef("LIBGL_ES", "2", "GL4ES: target GLES version (1 or 2)"),
    AngleEnvVarDef("LIBGL_GL", "21", "GL4ES: max OpenGL version (21 = GL 2.1)"),
    AngleEnvVarDef("LIBGL_NORMALIZE", "1", "GL4ES: normalize vertex data"),
    AngleEnvVarDef("LIBGL_FB", "3", "GL4ES: framebuffer mode (3 = FBO/pbuffer)"),
    AngleEnvVarDef("LIBGL_EGL", "", "GL4ES: path to EGL library (auto-set to ANGLE's libEGL.so)"),
    AngleEnvVarDef("LIBGL_GLES", "", "GL4ES: path to GLES library (auto-set to ANGLE's libGLESv2.so)"),
    AngleEnvVarDef("LIBGL_NOERROR", "1", "GL4ES: suppress GL error checks"),
    AngleEnvVarDef("LIBGL_SILENTSTUB", "1", "GL4ES: suppress stub function warnings"),
    AngleEnvVarDef("WINE_X11FORCEGLX", "1", "Force Wine to use GLX (needed for GL4ES)"),
    AngleEnvVarDef("BOX64_EMULATED_LIBS", "", "Do NOT add libGL here — gl4es is ARM64, must be natively wrapped by Box64"),
    AngleEnvVarDef("ANGLE_LOG_SEVERITY", "", "ANGLE log level (info/warning/error or empty)"),
    AngleEnvVarDef("LIBGL_DEBUG", "0", "GL4ES debug logging (0 or 1)"),
    AngleEnvVarDef("vblank_mode", "0", "Disable vblank sync"),
)

/**
 * State holder for ContainerConfigDialog. Built inside the dialog and passed to each tab composable.
 * Holds references to all mutable state and read-only data so tabs can read/write without 50+ parameters.
 */
class ContainerConfigState(
    val config: MutableState<ContainerData>,
    val graphicsDrivers: MutableState<MutableList<String>>,
    val bionicWineEntries: MutableState<List<String>>,
    val glibcWineEntries: MutableState<List<String>>,
    val wrapperVersions: MutableState<List<String>>,
    val dxvkVersionsAll: MutableState<List<String>>,
    val componentAvailability: MutableState<ManifestComponentHelper.ComponentAvailability?>,
    val showCustomResolutionDialog: MutableState<Boolean>,
    val customResolutionValidationError: MutableState<String?>,
    val vkMaxVersionIndex: MutableIntState,
    val imageCacheIndex: MutableIntState,
    val exposedExtIndices: MutableState<List<Int>>,
    val maxDeviceMemoryIndex: MutableIntState,
    val bionicDriverIndex: MutableIntState,
    val wrapperVersionIndex: MutableIntState,
    val presentModeIndex: MutableIntState,
    val resourceTypeIndex: MutableIntState,
    val bcnEmulationIndex: MutableIntState,
    val bcnEmulationTypeIndex: MutableIntState,
    val bcnEmulationCacheEnabled: MutableState<Boolean>,
    val disablePresentWaitChecked: MutableState<Boolean>,
    val syncEveryFrameChecked: MutableState<Boolean>,
    val sharpnessEffectIndex: MutableIntState,
    val sharpnessLevel: MutableIntState,
    val sharpnessDenoise: MutableIntState,
    val adrenotoolsTurnipChecked: MutableState<Boolean>,
    val emulator64Index: MutableIntState,
    val emulator32Index: MutableIntState,
    val screenSizeIndex: MutableIntState,
    val customScreenWidth: MutableState<String>,
    val customScreenHeight: MutableState<String>,
    val graphicsDriverIndex: MutableIntState,
    val dxWrapperIndex: MutableIntState,
    val dxvkVersionIndex: MutableIntState,
    val graphicsDriverVersionIndex: MutableIntState,
    val audioDriverIndex: MutableIntState,
    val gpuNameIndex: MutableIntState,
    val renderingModeIndex: MutableIntState,
    val videoMemIndex: MutableIntState,
    val mouseWarpIndex: MutableIntState,
    val externalDisplayModeIndex: MutableIntState,
    val languageIndex: MutableIntState,
    val showEnvVarCreateDialog: MutableState<Boolean>,
    val showAddDriveDialog: MutableState<Boolean>,
    val selectedDriveLetter: MutableState<String>,
    val pendingDriveLetter: MutableState<String>,
    val driveLetterMenuExpanded: MutableState<Boolean>,
    val screenSizes: List<String>,
    val baseGraphicsDrivers: List<String>,
    val dxWrappers: List<String>,
    val dxvkVersionsBase: List<String>,
    val vkd3dVersionsBase: List<String>,
    val audioDrivers: List<String>,
    val presentModes: List<String>,
    val resourceTypes: List<String>,
    val bcnEmulationEntries: List<String>,
    val bcnEmulationTypeEntries: List<String>,
    val sharpnessEffects: List<String>,
    val sharpnessDisplayItems: List<String>,
    val renderingModes: List<String>,
    val videoMemSizes: List<String>,
    val mouseWarps: List<String>,
    val externalDisplayModes: List<String>,
    val winCompOpts: List<String>,
    val box64Versions: List<String>,
    val wowBox64VersionsBase: List<String>,
    val box64BionicVersionsBase: List<String>,
    val fexcoreVersionsBase: List<String>,
    val fexcoreTSOPresets: List<String>,
    val fexcoreX87Presets: List<String>,
    val fexcoreMultiblockValues: List<String>,
    val startupSelectionEntries: List<String>,
    val turnipVersions: List<String>,
    val virglVersions: List<String>,
    val virglAngleVersions: List<String>,
    val zinkVersions: List<String>,
    val vortekVersions: List<String>,
    val adrenoVersions: List<String>,
    val sd8EliteVersions: List<String>,
    val containerVariants: List<String>,
    val bionicWineEntriesBase: List<String>,
    val glibcWineEntriesBase: List<String>,
    val emulatorEntries: List<String>,
    val bionicGraphicsDrivers: List<String>,
    val baseWrapperVersions: List<String>,
    val languages: List<String>,
    val dxvkOptions: ManifestComponentHelper.VersionOptionList,
    val vkd3dOptions: ManifestComponentHelper.VersionOptionList,
    val box64Options: ManifestComponentHelper.VersionOptionList,
    val box64BionicOptions: ManifestComponentHelper.VersionOptionList,
    val wowBox64Options: ManifestComponentHelper.VersionOptionList,
    val fexcoreOptions: ManifestComponentHelper.VersionOptionList,
    val wrapperOptions: ManifestComponentHelper.VersionOptionList,
    val bionicWineOptions: ManifestComponentHelper.VersionOptionList,
    val glibcWineOptions: ManifestComponentHelper.VersionOptionList,
    val dxvkManifestById: Map<String, ManifestEntry>,
    val vkd3dManifestById: Map<String, ManifestEntry>,
    val box64ManifestById: Map<String, ManifestEntry>,
    val wowBox64ManifestById: Map<String, ManifestEntry>,
    val fexcoreManifestById: Map<String, ManifestEntry>,
    val wrapperManifestById: Map<String, ManifestEntry>,
    val bionicWineManifestById: Map<String, ManifestEntry>,
    val glibcWineManifestById: Map<String, ManifestEntry>,
    val gpuCards: Map<Int, ContainerUtils.GpuInfo>,
    val box64Presets: List<Box86_64Preset>,
    val fexcorePresets: List<FEXCorePreset>,
    val gpuExtensions: List<String>,
    val inspectionMode: Boolean,
    val isBionicVariant: Boolean,
    // --- BEGIN ANGLE driver state ---
    val angleBackendIndex: MutableIntState,     // 0=Vulkan (only option — GLES backend not compiled)
    val angleLogsEnabled: MutableState<Boolean>, // ANGLE debug logging toggle
    val angleBackendEntries: List<String>,       // ["Vulkan"]
    /** Per-variable overrides for ANGLE env vars (key → value). Stored in graphicsDriverConfig as angleEnv_<KEY>. */
    val angleEnvOverrides: MutableState<Map<String, String>>,
    // --- END ANGLE driver state ---
    val nonDeletableDriveLetters: Set<String>,
    val availableDriveLetters: List<String>,
    val launchManifestInstall: (ManifestEntry, String, Boolean, ContentProfile.ContentType?, () -> Unit) -> Unit,
    val launchManifestContentInstall: (ManifestEntry, ContentProfile.ContentType, () -> Unit) -> Unit,
    val launchManifestDriverInstall: (ManifestEntry, () -> Unit) -> Unit,
    val getStartupSelectionOptions: () -> List<String>,
    val launchFolderPicker: () -> Unit,
    val getVersionsForDriver: () -> List<String>,
    val getVersionsForBox64: () -> ManifestComponentHelper.VersionOptionList,
    val applyScreenSizeToConfig: () -> Unit,
    val vkd3dForcedVersion: () -> String,
    val currentDxvkContext: () -> ManifestComponentHelper.DxvkContext,
) {
    /**
     * Returns the effective value of an ANGLE env var: the user override if set,
     * otherwise the default from [ANGLE_ENV_VAR_DEFS].
     */
    fun getAngleEnvValue(key: String): String {
        return angleEnvOverrides.value[key]
            ?: ANGLE_ENV_VAR_DEFS.firstOrNull { it.key == key }?.defaultValue
            ?: ""
    }

    /**
     * Updates an ANGLE env var override and persists it to graphicsDriverConfig.
     */
    fun setAngleEnvValue(key: String, value: String) {
        val newOverrides = angleEnvOverrides.value.toMutableMap()
        newOverrides[key] = value
        angleEnvOverrides.value = newOverrides
        // Persist to graphicsDriverConfig with "angleEnv_" prefix
        val cfg = KeyValueSet(config.value.graphicsDriverConfig)
        cfg.put("angleEnv_$key", value)
        config.value = config.value.copy(graphicsDriverConfig = cfg.toString())
    }

    /**
     * Loads ANGLE env var overrides from graphicsDriverConfig into the state map.
     * Called during initialization and after driver switch.
     */
    fun loadAngleEnvOverridesFromConfig() {
        val cfg = KeyValueSet(config.value.graphicsDriverConfig)
        val overrides = mutableMapOf<String, String>()
        for (def in ANGLE_ENV_VAR_DEFS) {
            val stored = cfg.get("angleEnv_${def.key}")
            if (stored.isNotEmpty()) {
                overrides[def.key] = stored
            }
        }
        angleEnvOverrides.value = overrides
    }

    /**
     * Clears all ANGLE env var overrides from graphicsDriverConfig and resets state.
     */
    fun clearAngleEnvOverrides() {
        val cfg = KeyValueSet(config.value.graphicsDriverConfig)
        for (def in ANGLE_ENV_VAR_DEFS) {
            cfg.remove("angleEnv_${def.key}")
        }
        config.value = config.value.copy(graphicsDriverConfig = cfg.toString())
        angleEnvOverrides.value = emptyMap()
    }
}
