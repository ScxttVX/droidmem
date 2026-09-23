#!/bin/bash
NDK_BIN="C:/Users/VX/AppData/Local/Android/Sdk/ndk/25.1.8937393/toolchains/llvm/prebuilt/windows-x86_64/bin"
CXX="$NDK_BIN/aarch64-linux-android21-clang++"
cd "$(dirname "$0")"
echo "[*] Compiling with: $CXX"
rm -f main.o main.exe main
$CXX -g -Wall -O2 -static -o main.o -c main.cpp 2>&1 && \
$CXX -g -Wall -O2 -static -o main main.o 2>&1 && \
echo "[+] Build OK: main" || echo "[-] Build FAILED"
