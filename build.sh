#!/bin/sh
# Needs a MinGW-w64 x86_64 cross compiler (Debian/Ubuntu: apt install gcc-mingw-w64-x86-64,
# macOS: brew install mingw-w64).
set -e
cd "$(dirname "$0")"
python3 src/gen.py
x86_64-w64-mingw32-gcc -O2 -shared -nostdlib -e DllMain -Ibuild \
    -o build/iphlpapi.dll src/proxy.c build/thunks.S build/iphlpapi.def -lkernel32
echo "built: build/iphlpapi.dll"
