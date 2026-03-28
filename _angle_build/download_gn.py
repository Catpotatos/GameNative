#!/usr/bin/env python3
"""Download GN binary and prepare for ANGLE build."""
import subprocess
import os
import zipfile
import urllib.request
import shutil

gn_dir = "/home/catarina/angle_build/angle/buildtools/linux64"
gn_path = os.path.join(gn_dir, "gn")
gn_zip = os.path.join(gn_dir, "gn.zip")

# Remove old empty gn
if os.path.exists(gn_path):
    os.chmod(gn_path, 0o755)
    os.remove(gn_path)
    print("Removed old empty gn")

# Download GN
print("Downloading GN binary...")
url = "https://chrome-infra-packages.appspot.com/dl/gn/gn/linux-amd64/+/latest"
urllib.request.urlretrieve(url, gn_zip)
print(f"Downloaded to {gn_zip}")

# Extract
print("Extracting...")
with zipfile.ZipFile(gn_zip, 'r') as z:
    z.extractall(gn_dir)
os.chmod(gn_path, 0o755)
os.remove(gn_zip)

# Verify
result = subprocess.run([gn_path, "--version"], capture_output=True, text=True)
print(f"GN version: {result.stdout.strip()}")
print("GN ready!")
