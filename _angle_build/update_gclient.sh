#!/bin/bash
# Update .gclient to point to chromium/7748
cd /home/catarina/angle_build

cat > .gclient << 'EOF'
solutions = [
  {
    "name": "angle",
    "url": "https://chromium.googlesource.com/angle/angle.git@chromium/7748",
    "deps_file": "DEPS",
    "managed": False,
    "custom_deps": {},
  },
]
target_os = ["android"]
EOF

echo "Updated .gclient:"
cat .gclient

