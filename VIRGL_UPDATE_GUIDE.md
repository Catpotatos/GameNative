# Updating VirGL Renderer in GameNative

## Overview

The VirGL renderer in GameNative (`app/src/main/cpp/virglrenderer/`) is based on **Mesa 23.1.9** with extensive Android/GLES modifications by Bruno. This guide walks through updating it to a newer Mesa version.

**Recommended target: Mesa 24.3.x** (latest 24.3 point release).

### Why 24.3.x instead of 26.0?

- Mesa 25.x+ is actively removing TGSI shader paths in favor of NIR. This project's `vrend_shader.c` is a 6750-line TGSI→GLSL translator. If TGSI support is gutted upstream, porting becomes dramatically harder.
- 24.3.x still has TGSI paths intact, with ~1.5 years of bug fixes over 23.1.9.
- An incremental approach (23.1.9 → 24.3.x → 25.x later) is much safer.

---

## Architecture Reference

### File Mapping: Your Project ↔ Upstream Mesa

| Your project path | Upstream Mesa path |
|---|---|
| `src/vrend_renderer.c` | `src/virgl/vrend_renderer.c` |
| `src/vrend_renderer.h` | `src/virgl/vrend_renderer.h` |
| `src/vrend_shader.c` | `src/virgl/vrend_shader.c` |
| `src/vrend_shader.h` | `src/virgl/vrend_shader.h` |
| `src/vrend_decode.c` | `src/virgl/vrend_decode.c` |
| `src/vrend_formats.c` | `src/virgl/vrend_formats.c` |
| `src/vrend_blitter.c` | `src/virgl/vrend_blitter.c` |
| `src/vrend_blitter.h` | `src/virgl/vrend_blitter.h` |
| `src/vrend_object.c` | `src/virgl/vrend_object.c` |
| `src/vrend_object.h` | `src/virgl/vrend_object.h` |
| `src/iov.c` | `src/virgl/iov.c` |
| `src/vrend_iov.h` | `src/virgl/vrend_iov.h` |
| `src/vrend_strbuf.h` | `src/virgl/vrend_strbuf.h` |
| `src/virgl_hw.h` | `src/virgl/virgl_hw.h` |
| `src/virgl_protocol.h` | `src/virgl/virgl_protocol.h` |
| `src/vrend_util.h` | **CUSTOM** (no upstream equivalent) |
| `src/gallium/...` | `src/gallium/...` (same paths) |
| `server/*` | **CUSTOM** (no upstream equivalent) |

### Bruno's Key Modifications (Categories)

#### 1. `virgl_client *` Pass-Through Pattern (MOST PERVASIVE)
Upstream virglrenderer uses **global state** (`static struct vrend_state vrend_state;`). Bruno modified it to pass a `struct virgl_client *client` parameter through ALL function call chains, with per-client state:

```c
// UPSTREAM (global state):
int vrend_renderer_init(struct vrend_if_cbs *cbs);
void vrend_renderer_fini(void);

// BRUNO'S MODIFICATION (per-client state):
int vrend_renderer_init(struct virgl_client *client, struct vrend_if_cbs *cbs);
void vrend_renderer_fini(struct virgl_client *client);
```

The `virgl_client` struct (defined in `server/virgl_server.h`) holds:
- `struct vrend_state *vrend_state` - per-client renderer state
- `struct util_hash_table *res_hash` - per-client resource hash
- `struct vrend_decode_ctx *dec_ctx[VREND_MAX_CTX]` - per-client decode contexts
- `struct vrend_blitter_ctx *vrend_blit_ctx` - per-client blitter context

This affects **100+ function signatures** across `vrend_renderer.c`, `vrend_decode.c`, `vrend_blitter.c`.

#### 2. `vrend_util.h` - Complete Replacement
This file is entirely custom. It:
- Includes GLES headers instead of desktop GL: `<GLES2/gl2.h>`, `<GLES3/gl3.h>`, etc.
- Defines missing GL constants: `GL_TEXTURE_1D`, `GL_TEXTURE_RECTANGLE`, `GL_TEXTURE_1D_ARRAY`, `GL_QUADS`, `GL_QUAD_STRIP`, `GL_POLYGON`
- Redirects `printf` to Android logcat: `#define printf(...) __android_log_print(...)`
- Includes `<jni.h>` and `<android/log.h>`
- Provides `vrend_gl_version()`, `vrend_has_gl_extension()`, `vrend_get_glsl_version()` implementations

#### 3. GLES Feature Adjustments in `vrend_renderer.c`
The feature capabilities table and initialization code are tuned for GLES 3.x:
- Feature checks reference GLES versions/extensions instead of desktop GL
- Some features are force-disabled (e.g., `GL_TEXTURE_1D` support)
- Blit operations have GLES-specific workarounds
- Texture storage uses GLES-compatible paths

#### 4. Shader Translation GLES Paths in `vrend_shader.c`
- RECT texture sampling emulated via coordinate normalization
- `writeonly` qualifiers on image declarations
- GLSL ES version strings instead of desktop GLSL
- Various GLES-specific shader workarounds

#### 5. Custom Server Layer (`server/`)
These files are NOT from upstream Mesa/virglrenderer at all:
- `virgl_server.c` - JNI entry points, connection/request handling
- `virgl_server_renderer.c` - EGL context management, resource CRUD, transfers
- `virgl_server_shm.c` - Android shared memory (memfd_create)
- `virgl_server.h` - Struct definitions, JNI info
- `virgl_server_protocol.h` - Wire protocol constants
- `virgl_server_shm.h` - SHM helper defines

These generally don't need changing unless `vrend_renderer.h` API signatures change.

---

## Step-by-Step Update Process

### Prerequisites

- **Git** installed
- **WSL2** (Ubuntu) recommended for diffing/merging (or use Git Bash on Windows)
- **Android Studio** with NDK installed
- A visual diff/merge tool (e.g., Beyond Compare, VS Code diff, Meld via WSL)

### Step 1: Set Up Working Repos

```bash
# Create a workspace for the update
mkdir ~/virgl-update && cd ~/virgl-update

# Clone Mesa
git clone --no-checkout https://gitlab.freedesktop.org/mesa/mesa.git
cd mesa

# Get the two versions we need
git checkout mesa-23.1.9 -- src/virgl/ src/gallium/
cp -r src/virgl src/gallium ~/virgl-update/mesa-23.1.9-virgl/
git checkout -- .

git checkout mesa-24.3.4 -- src/virgl/ src/gallium/  # or latest 24.3.x tag
cp -r src/virgl src/gallium ~/virgl-update/mesa-24.3.x-virgl/
```

### Step 2: Generate Bruno's Modification Diff

```bash
cd ~/virgl-update

# Compare upstream 23.1.9 virgl files vs Bruno's modified versions
# (Copy current project files to ~/virgl-update/current-virgl/ first)
diff -ruN mesa-23.1.9-virgl/virgl/ current-virgl/src/ > bruno_mods_core.patch
diff -ruN mesa-23.1.9-virgl/gallium/ current-virgl/gallium/ > bruno_mods_gallium.patch
```

Review these patches carefully. Categorize each hunk into:
- `virgl_client` parameter additions
- GLES header/constant changes
- Feature table adjustments
- Shader translation changes
- Bug fixes / other

### Step 3: Generate Upstream Change Diff

```bash
# See what changed upstream between versions
cd mesa
git diff mesa-23.1.9..mesa-24.3.4 -- src/virgl/ > upstream_virgl_changes.patch
git diff mesa-23.1.9..mesa-24.3.4 -- src/gallium/auxiliary/ src/gallium/include/ > upstream_gallium_changes.patch

# Check for new/removed files
git diff mesa-23.1.9..mesa-24.3.4 --stat -- src/virgl/
```

### Step 4: Three-Way Merge (The Core Work)

For each source file, perform a three-way merge:

```bash
# Example for vrend_renderer.c:
git merge-file \
  current-virgl/src/vrend_renderer.c \
  mesa-23.1.9-virgl/virgl/vrend_renderer.c \
  mesa-24.3.x-virgl/virgl/vrend_renderer.c
```

This will put merge markers (`<<<<<<<`) in the file where conflicts exist. Resolve them manually.

**Priority order for merging** (start with smallest/simplest files):
1. `virgl_hw.h` - Usually additive (new format enums)
2. `virgl_protocol.h` - Usually additive (new protocol commands)
3. `iov.c` / `vrend_iov.h` - Small, rarely changes much
4. `vrend_object.c` / `vrend_object.h` - Small
5. `vrend_strbuf.h` - Small
6. `vrend_blitter.c` / `vrend_blitter.h` - Medium
7. `vrend_formats.c` - Medium (format table changes)
8. `vrend_decode.c` - Large, but mostly mechanical
9. `vrend_shader.c` / `vrend_shader.h` - Very large, complex
10. `vrend_renderer.c` / `vrend_renderer.h` - Largest, most complex

**`vrend_util.h` does NOT need merging** — it's entirely custom. Keep yours as-is, but check if new GL constants are needed.

### Step 5: Update Gallium Dependencies

Replace all files under `src/gallium/` with Mesa 24.3.x equivalents:

```bash
# Check for new files needed
diff <(find mesa-23.1.9-virgl/gallium -name "*.c" -o -name "*.h" | sort) \
     <(find mesa-24.3.x-virgl/gallium -name "*.c" -o -name "*.h" | sort)
```

Update `CMakeLists.txt` if files were added/removed.

### Step 6: Update CMakeLists.txt

Check if any new `.c` files need to be compiled:

```bash
# In mesa-24.3.x/src/virgl/, check for new .c files
ls mesa-24.3.x-virgl/virgl/*.c | sort

# Compare against current CMakeLists.txt source list
```

Also check for new `#include` directives in the updated files that reference headers not currently in the include paths.

### Step 7: Update Server Code (If Needed)

Only if `vrend_renderer.h` function signatures changed. Check:
- `vrend_renderer_init` signature
- `vrend_renderer_fill_caps` signature
- `vrend_renderer_resource_create` / `vrend_renderer_resource_create_args` struct
- `vrend_renderer_transfer_iov` / `vrend_transfer_info` struct
- `vrend_renderer_create_fence` signature
- `vrend_renderer_check_fences` signature
- `vrend_decode_block` signature

If any of these changed, update `server/virgl_server_renderer.c` accordingly.

### Step 8: Build and Fix Errors

```bash
# In Android Studio, or from command line:
cd /path/to/GameNative-mine
./gradlew assembleDebug 2>&1 | tee build_output.txt
```

Common error categories:
1. **Missing symbols** — A new function/struct/macro was referenced. Find it in the Mesa source and add the relevant file.
2. **Changed struct fields** — A struct gained/lost fields. Update initializers.
3. **Missing GL constants** — New GL features not in GLES. Add `#define` stubs to `vrend_util.h`.
4. **Implicit function declarations** — Function moved to a different file or renamed.
5. **`virgl_client *` missing** — Forgot to add the parameter in a newly-added upstream function.

### Step 9: Test

1. Build APK and install on device
2. Launch with VirGL graphics driver selected
3. Test with simple Wine/DXVK applications first
4. Check logcat for VirGL-related errors:
   ```bash
   adb logcat -s "System.out" "VirGLRendererComponent"
   ```
5. Test with progressively more complex games

---

## Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|---|---|---|---|
| TGSI path removed in target | Low (24.3.x) | Critical | Verify before starting: `grep -r "tgsi" mesa-24.3.x/src/virgl/` |
| `virgl_client` merge conflicts | High | Medium | Systematic approach: merge upstream first, then re-apply client pattern |
| New gallium dependencies | Medium | Low | Check all `#include` directives; add missing files |
| GLES incompatibility in new features | Medium | Medium | Can disable unsupported features in caps/feature table |
| Runtime regressions | Medium | High | Keep old code in a git branch; A/B test |

## Fallback: Cherry-Pick Bug Fixes Only

If a full update is too risky, cherry-pick specific fixes:

```bash
cd mesa
# List all virgl commits between versions:
git log --oneline mesa-23.1.9..mesa-24.3.4 -- src/virgl/

# For each interesting commit, generate a patch:
git format-patch -1 <commit-hash> -- src/virgl/

# Manually apply relevant hunks to your modified files
```

---

## Quick Reference: File Sizes

| File | Lines | Complexity |
|---|---|---|
| `vrend_renderer.c` | 7,366 | Very High |
| `vrend_shader.c` | 6,750 | Very High |
| `vrend_decode.c` | ~1,500 | High |
| `vrend_renderer.h` | 406 | Medium |
| `vrend_formats.c` | ~400 | Medium |
| `vrend_blitter.c` | ~840 | Medium |
| `vrend_object.c` | ~100 | Low |
| `iov.c` | ~50 | Low |
| `virgl_hw.h` | 536 | Low (enum list) |
| `virgl_protocol.h` | ~350 | Low (defines) |
| Gallium utilities | ~20 files | Low (mostly untouched) |
| Server files | 4 files | N/A (custom, usually unchanged) |

