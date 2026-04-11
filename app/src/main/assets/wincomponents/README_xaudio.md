# XAudio — Audio DLL Override

There are **two** xaudio archives in this folder:

| File | Contents |
|------|----------|
| `xaudio.tzst` | Wine builtin stubs (used by the Win Components toggle) |
| `xaudio_native.tzst` | FAudio-linked native DLLs (used by `WINEDLLOVERRIDES`) |

The `xaudio_native.tzst` archive contains native XAudio2 and XAPOFX DLLs that
provide DirectX audio support for games relying on the XAudio2 API.

## DLL details

These are **Wine PE DLLs** with [FAudio](https://github.com/FNA-XNA/FAudio)
26.04 statically linked in. They are architecture-neutral PE binaries:

- On **x86_64** hosts Wine/Proton loads them as-is.
- On **ARM64EC** hosts the Wine WoW64 layer handles the translation
  transparently.

Compatible with **Proton 9/10** and **Wine 9+** builds — just drop them
into the appropriate prefix directories (`system32/` / `syswow64/`).

## How it works

XAudio is available both as a **Win Component** toggle (the traditional way)
and via the `WINEDLLOVERRIDES` environment variable (like OpenAL). The two
paths use **different archives**:

- **Win Components toggle** → extracts `xaudio.tzst` (Wine builtin stubs)
- **WINEDLLOVERRIDES** → extracts `xaudio_native.tzst` (FAudio-linked native DLLs)

### Common presets (available in the dropdown)

| WINEDLLOVERRIDES value                                           | Effect                                                       |
|------------------------------------------------------------------|--------------------------------------------------------------|
| `xaudio2_7=native,builtin`                                      | Extracts DLLs, Wine prefers native xaudio2_7                 |
| `xaudio2_8=native,builtin`                                      | Extracts DLLs, Wine prefers native xaudio2_8                 |
| `xaudio2_9=native,builtin`                                      | Extracts DLLs, Wine prefers native xaudio2_9                 |
| `xaudio2_7=native,builtin;xapofx1_5=native,builtin`             | Extracts DLLs, xaudio2 + xapofx native                      |
| `xaudio2_7=native,builtin;xaudio2_8=native,builtin;xaudio2_9=native,builtin` | All three xaudio2 versions native           |
| *(not set)*                                                      | No override — Wine uses its own builtin (default)            |

Users can also type any custom `WINEDLLOVERRIDES` value in the create dialog.

### Detection logic

At boot, the app:
1. Reads `WINEDLLOVERRIDES` from the container's env vars
2. If the value mentions `xaudio2_`, `x3daudio1_`, `xactengine`, or
   `xapofx1_`, extracts `xaudio_native.tzst` into a **staging directory**
   at `drive_c/native_dlls/xaudio/` (never directly into system32/syswow64),
   then copies the DLLs from staging into `drive_c/windows/system32/`
   and `drive_c/windows/syswow64/`
3. When `ALWAYS_REEXTRACT` is enabled, re-extracts on every boot
4. Passes `WINEDLLOVERRIDES` directly to Wine — no translation needed
5. **On disable** (when xaudio overrides are removed from WINEDLLOVERRIDES):
   restores Proton's builtin `xaudio2_7/8/9.dll` via `restoreOriginalDllFiles`,
   and deletes the unique `xapofx1_5.dll` from system32/syswow64
4. Passes `WINEDLLOVERRIDES` directly to Wine — no translation needed

Extraction is logged via Timber with the `XAudio` prefix — filter logcat
with `XAudio check` or `XAudio —` to see the full decision trace.

### Troubleshooting — no audio with xaudio override

1. **Check the logs first** — look for `XAudio check` and `XAudio —`
   lines in logcat. They show:
   - The parsed `WINEDLLOVERRIDES` value
   - Whether `needsXaudioDlls` resolved to true
   - Whether extraction ran and succeeded
   - Whether `xaudio2_7.dll` exists on disk after extraction

2. **Verify the audio backend** — the DLL chain is:
   `Game → xaudio2_7.dll (native) → FAudio → PulseAudio (PULSE_SERVER)`
   If the PulseAudio socket doesn't exist or the server isn't running,
   nothing will produce sound regardless of the DLL override.

3. **Test without override** — remove `WINEDLLOVERRIDES` for xaudio
   and check if Wine's builtin xaudio2 produces sound. If it doesn't,
   the problem is the audio backend, not the DLLs.

4. **Confirm DLL load order** — set `WINEDEBUG=+loaddll` (instead of
   `-all`) to confirm whether Wine loads native or builtin xaudio2_7.dll.

### Recommended setting

Use `xaudio2_7=native,builtin` for most games. XAudio 2.7 is the most
commonly used version in DirectX 9/10/11 games. Modern titles (Windows 8+)
may need `xaudio2_8` or `xaudio2_9`.

All xaudio2 versions (0–9) exist as Wine builtins in the Wine prefix
(listed in `common_dlls.json`). The `xaudio.tzst` archive provides
native DLLs to override them.

## Archive contents

```
system32/          (64-bit PE DLLs)
  xaudio2_7.dll    1.7 MB
  xaudio2_8.dll    1.8 MB
  xaudio2_9.dll    1.8 MB
  xapofx1_5.dll    301 KB

syswow64/          (32-bit PE DLLs)
  xaudio2_7.dll    1.4 MB
  xaudio2_8.dll    1.5 MB
  xaudio2_9.dll    1.5 MB
  xapofx1_5.dll    241 KB
```

## Relationship to Win Components

The `xaudio` entry in `wincomponents.json` handles the same DLLs via the
traditional Win Components toggle. This `WINEDLLOVERRIDES` path is an
alternative that lets users control xaudio extraction per-container without
changing Win Components, and also sets up the correct Wine override mode.

---

## Building XAudio2/XAPOFX DLLs (Wine + FAudio)

### Prerequisites

- **MSYS2** with MinGW-w64 toolchains:
  ```
  pacman -S mingw-w64-x86_64-gcc mingw-w64-i686-gcc make autoconf bison flex zstd
  ```
- **Wine source** (tested with 11.6)
- **FAudio source** (tested with 26.04)

### Steps

All commands run in the **MINGW64** shell (`MSYSTEM=MINGW64`).

#### 1. Copy FAudio sources

Copy the FAudio 26.04 sources into Wine's bundled location:

```bash
cp FAudio-26.04/src/*.c FAudio-26.04/src/*.h wine/libs/faudio/src/
cp FAudio-26.04/include/*.h wine/libs/faudio/include/
```

#### 2. Fix duplicate symbol

In `libs/faudio/src/FAudio_platform_win32_wmadec.c`, change:

```c
DEFINE_MEDIATYPE_GUID(MFAudioFormat_XMAudio2, FAUDIO_FORMAT_XMAUDIO2);
```

to:

```c
extern const GUID MFAudioFormat_XMAudio2;
```

This avoids a duplicate definition (the symbol is already defined in
`FAudio_platform_win32.c`).

#### 3. Patch configure

In the generated `configure` script, find the line (around line 6627):

```sh
eval "${HOST_ARCH}_CC=$CC"
```

Change it to:

```sh
eval "${HOST_ARCH}_CC=\${${HOST_ARCH}_CC:-$CC}"
```

This prevents the `mingw32*|cygwin*` case from overriding the
cross-compiler passed via `x86_64_CC`.

#### 4. Configure

Run an in-source configure (out-of-source builds cause MSYS2 path
translation issues with makedep):

```bash
./configure \
  x86_64_CC=x86_64-w64-mingw32-gcc \
  --enable-archs=i386,x86_64 \
  --with-mingw \
  --disable-tests \
  --without-freetype --without-x # ...disable other unneeded deps
```

#### 5. Remove `--without-dlltool` from Makefile

```bash
sed -i 's/ --without-dlltool//g' Makefile
```

Without this fix, winebuild spawns gcc once per export symbol instead of
using dlltool, which fails on MSYS2.

#### 6. Build

Build the FAudio static libraries, then the DLLs:

```bash
make libs/faudio/i386-windows/libfaudio.a libs/faudio/x86_64-windows/libfaudio.a
make dlls/xaudio2_7/all dlls/xaudio2_8/all dlls/xaudio2_9/all dlls/xapofx1_5/all
```

### Build output

| DLL | 64-bit | 32-bit |
|-----|--------|--------|
| xaudio2_7.dll | `dlls/xaudio2_7/x86_64-windows/` | `dlls/xaudio2_7/i386-windows/` |
| xaudio2_8.dll | `dlls/xaudio2_8/x86_64-windows/` | `dlls/xaudio2_8/i386-windows/` |
| xaudio2_9.dll | `dlls/xaudio2_9/x86_64-windows/` | `dlls/xaudio2_9/i386-windows/` |
| xapofx1_5.dll | `dlls/xapofx1_5/x86_64-windows/` | `dlls/xapofx1_5/i386-windows/` |

### Bundling

Place 64-bit DLLs in `system32/` and 32-bit in `syswow64/`, then archive:

```bash
tar -cf xaudio_native.tzst --zstd system32/ syswow64/
```

Place the resulting `xaudio_native.tzst` in `app/src/main/assets/wincomponents/`
alongside the existing `xaudio.tzst` (which stays as-is for Win Components).

