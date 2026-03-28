#!/usr/bin/env python3
"""Check gl4es binary for EGL/GLES library name strings."""
import os, sys

path = os.path.join(os.environ.get("TEMP", "/tmp"), "gl4es_check", "opt", "gl4es", "lib", "libGL.so.1")
if not os.path.exists(path):
    # Try extracting from assets
    import subprocess
    asset = r"C:\Users\Catarina\StudioProjects\GameNative-mine\app\src\main\assets\graphics_driver\gl4es-bionic-1.1.7.tzst"
    outdir = os.path.join(os.environ.get("TEMP", "/tmp"), "gl4es_check2")
    os.makedirs(outdir, exist_ok=True)
    subprocess.run(["tar", "-xf", asset, "-C", outdir])
    path = os.path.join(outdir, "opt", "gl4es", "lib", "libGL.so.1")

with open(path, "rb") as f:
    data = f.read()

print(f"File: {path}")
print(f"Size: {len(data)} bytes")
print()

searches = [
    "libEGL.so",
    "libEGL.so.1",
    "libGLESv2.so",
    "libGLESv2.so.2",
    "libGLESv1_CM.so",
    "LIBGL_EGL",
    "LIBGL_GLES",
    "libEGL",
    "libGLES",
    "eglGetDisplay",
    "eglInitialize",
    "eglChooseConfig",
    "glXGetFBConfigs",
    "glXChooseVisual",
]

for s in searches:
    needle = s.encode("ascii")
    found = needle in data
    extra = ""
    if found:
        idx = data.find(needle)
        # Get surrounding context as printable chars
        start = max(0, idx - 15)
        end = min(len(data), idx + len(needle) + 25)
        ctx_bytes = data[start:end]
        ctx = "".join(chr(b) if 32 <= b < 127 else "|" for b in ctx_bytes)
        extra = f"  @ offset {idx}: ...{ctx}..."
    print(f"  {s:25s} : {'YES' if found else 'NO'}{extra}")

