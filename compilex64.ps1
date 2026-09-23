$NDK = "C:\Users\VX\AppData\Local\Android\Sdk\ndk\25.1.8937393\toolchains\llvm\prebuilt\windows-x86_64\bin"
$CXX = "$NDK\x86_64-linux-android21-clang++.cmd"
Set-Location "X:\hackings\droidmem\droidmem\um"
Write-Host "[*] Compilando x86_64 (emuladores) com: $CXX"
& $CXX -g -Wall -O2 -static -o main.o -c main.cpp
if ($LASTEXITCODE -ne 0) { Write-Host "[-] Falha na compilação"; exit 1 }
& $CXX -g -Wall -O2 -static -o main64 main.o
if ($LASTEXITCODE -ne 0) { Write-Host "[-] Falha no link"; exit 1 }
Write-Host "[+] Build OK: main64"
Write-Host ""
Write-Host "Deploy no emulador:"
Write-Host '  $ADB push main64 /data/local/tmp/'
Write-Host '  $ADB shell chmod 755 /data/local/tmp/main64'
Write-Host '  $ADB shell "su -c /data/local/tmp/main64 com.dts.freefireth libil2cpp.so"'
