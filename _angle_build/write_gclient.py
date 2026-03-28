#!/usr/bin/env python3
import sys

content = """solutions = [
  {
    "name": "angle",
    "url": "https://chromium.googlesource.com/angle/angle.git@chromium/7736",
    "deps_file": "DEPS",
    "managed": False,
    "custom_deps": {},
  },
]
target_os = ["android"]
"""

path = "/home/catarina/angle_build/.gclient"
with open(path, "w") as f:
    f.write(content)
print("Written to", path)
with open(path, "r") as f:
    print(f.read())

