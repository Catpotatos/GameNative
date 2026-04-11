# DSOAL — DirectSound → OpenAL Soft

There are **two** dsound-related archives in this folder:

| File | Contents |
|------|----------|
| `directsound.tzst` | Wine builtin `dsound.dll` (used by the Win Components toggle) |
| `dsoal.tzst` | DSOAL native `dsound.dll` + `dsoal-aldrv.dll` (used by `WINEDLLOVERRIDES`) |

DSOAL replaces Wine's internal DirectSound mixer with one that routes
all DirectSound output through OpenAL Soft. This **bypasses Wine's dsound
mixer entirely**, which eliminates a common source of crackling on
Android's less-predictable audio scheduler.

## Why DSOAL?

Wine's builtin dsound has its own mixer that runs before PulseAudio.
Setting `PULSE_LATENCY_MSEC=144` helps PulseAudio but doesn't fix
crackling that happens inside Wine's dsound mixer layer. DSOAL skips
that mixer and goes straight through OpenAL Soft → PulseAudio.

Audio chain comparison:

```
Without DSOAL:  Game → dsound.dll (Wine mixer) → PulseAudio → AAudio
With DSOAL:     Game → dsound.dll (DSOAL) → dsoal-aldrv.dll (OpenAL Soft) → PulseAudio → AAudio
```

## Archive contents

```
system32/              (64-bit)
  dsound.dll           DSOAL wrapper
  dsoal-aldrv.dll      OpenAL Soft (renamed from soft_oal.dll)

syswow64/              (32-bit)
  dsound.dll           DSOAL wrapper
  dsoal-aldrv.dll      OpenAL Soft (renamed from soft_oal.dll)
```

## How it works

### Detection logic

At boot, the app:
1. Reads `WINEDLLOVERRIDES` from the container's env vars
2. If the value mentions `dsound`, extracts `dsoal.tzst` into a
   **staging directory** at `drive_c/native_dlls/dsoal/` (never directly
   into system32/syswow64), then copies the DLLs from staging into
   `drive_c/windows/system32/` and `drive_c/windows/syswow64/`
3. Writes an `alsoft.ini` config to `drive_c/windows/` with buffer
   settings tuned for Android
4. When `ALWAYS_REEXTRACT` is enabled, re-extracts on every boot
5. Passes `WINEDLLOVERRIDES` directly to Wine
6. **On disable** (when `dsound` is removed from WINEDLLOVERRIDES):
   restores Proton's builtin `dsound.dll` via `restoreOriginalDllFiles`,
   deletes the unique `dsoal-aldrv.dll` from system32/syswow64, and
   removes `alsoft.ini`

Extraction is logged via Timber with the `DSOAL` prefix — filter logcat
with `DSOAL check` or `DSOAL —` to see the full decision trace.

### Common presets (available in the dropdown)

| WINEDLLOVERRIDES value                         | Effect |
|------------------------------------------------|--------|
| `dsound=native,builtin`                        | DSOAL only — DirectSound through OpenAL Soft |
| `dsound=native,builtin;openal32=native,builtin`| DSOAL + system OpenAL (covers both APIs) |
| *(not set)*                                    | Wine uses its own builtin dsound (default) |

### alsoft.ini

After extracting the DLLs, the app writes `drive_c/windows/alsoft.ini`
with conservative buffer settings:

```ini
[general]
period_size = 1024
periods = 4
sources = 256
```

This gives OpenAL Soft more buffer headroom, reducing crackling on
Android. The `sources` value sets the max simultaneous sound sources.

These values can be manually adjusted by editing the file on-device at:
```
/data/user/0/<package>/files/imagefs/home/xuser/.wine/drive_c/windows/alsoft.ini
```

## Relationship to other components

- **`directsound.tzst`** (Win Components toggle) — extracts Wine's
  builtin `dsound.dll`. If Win Components has directsound enabled,
  that extraction runs first (line ~3790). The DSOAL extraction runs
  after and overwrites with the native DSOAL dsound.dll.

- **`openal.tzst`** (WINEDLLOVERRIDES `openal32=native,builtin`) —
  extracts OpenAL Soft as `openal32.dll` + `soft_oal.dll`. Independent
  of DSOAL. Use the combined preset if you want both.

- **The existing `openal.tzst` is NOT modified** — DSOAL uses its own
  copy of OpenAL Soft renamed to `dsoal-aldrv.dll` inside `dsoal.tzst`.

## Recommended setting

Use `dsound=native,builtin` for games that use DirectSound and have
audio crackling with Wine's builtin dsound. Most older games (pre-2010)
and many indie titles use DirectSound.

For games that also load OpenAL directly (e.g., some UE3 titles),
use the combined preset:
`dsound=native,builtin;openal32=native,builtin`

## Building dsoal.tzst

### Prerequisites

- **DSOAL** `dsound.dll` — get pre-built releases from
  https://github.com/kcat/dsoal/releases or build from source
  (both 32-bit and 64-bit)
- **OpenAL Soft** — same DLLs used in `openal.tzst`, renamed

### Steps

1. Create a staging directory:
   ```
   system32/dsound.dll          (64-bit DSOAL)
   system32/dsoal-aldrv.dll     (64-bit OpenAL Soft, renamed from soft_oal.dll)
   syswow64/dsound.dll          (32-bit DSOAL)
   syswow64/dsoal-aldrv.dll     (32-bit OpenAL Soft, renamed from soft_oal.dll)
   ```

2. The `dsoal-aldrv.dll` files are simply OpenAL Soft `soft_oal.dll`
   renamed. You can copy them from the same OpenAL Soft builds used
   for `openal.tzst`:
   ```bash
   cp soft_oal_win64.dll system32/dsoal-aldrv.dll
   cp soft_oal_win32.dll syswow64/dsoal-aldrv.dll
   ```

3. Create the archive:
   ```bash
   tar -cf dsoal.tzst --zstd system32/ syswow64/
   ```

4. Place `dsoal.tzst` in `app/src/main/assets/wincomponents/`

### Troubleshooting

1. **Check logs** — look for `DSOAL check` and `DSOAL —` in logcat
2. **Verify DLL sizes** — DSOAL's dsound.dll should be much larger
   than Wine's builtin (~200KB+ vs ~50KB)
3. **Confirm load order** — set `WINEDEBUG=+loaddll` to verify Wine
   loads native dsound.dll
4. **Test without DSOAL** — remove `dsound` from WINEDLLOVERRIDES to
   check if Wine's builtin dsound produces audio at all
5. **Check alsoft.ini** — if audio still crackles, increase
   `period_size` to 2048 or `periods` to 8

