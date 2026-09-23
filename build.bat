@echo off
REM build.bat — Compilar droidmem (ARM64)
set NDK_BIN=C:\Users\VX\AppData\Local\Android\Sdk\ndk\25.1.8937393\toolchains\llvm\prebuilt\windows-x86_64\bin
set CXX=%NDK_BIN%\aarch64-linux-android21-clang++.exe
cd /d X:\hackings\droidmem\droidmem\um
echo [*] Compilando ARM64 com: %CXX%
del /q main.o main.exe main 2>nul
"%CXX%" -g -Wall -O2 -static -o main.o -c main.cpp
if errorlevel 1 goto :fail
"%CXX%" -g -Wall -O2 -static -o main main.o
if errorlevel 1 goto :fail
echo [+] Build OK: main
goto :end
:fail
echo [-] Falha na compilação
:end
