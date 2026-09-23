$NDK = "C:\Users\VX\AppData\Local\Android\Sdk\ndk\25.1.8937393\toolchains\llvm\prebuilt\windows-x86_64\bin"
$CXX = "$NDK\x86_64-linux-android21-clang++.cmd"
Set-Location "X:\hackings\droidmem\droidmem\um"
Write-Host "[*] Compilando test_proc x86_64 com: $CXX"
& $CXX -g -Wall -O2 -static -o test_proc.o -c test_proc.cpp
if ($LASTEXITCODE -ne 0) { Write-Host "[-] Falha na compilação"; exit 1 }
& $CXX -g -Wall -O2 -static -o test_proc test_proc.o
if ($LASTEXITCODE -ne 0) { Write-Host "[-] Falha no link"; exit 1 }
Write-Host "[+] Build OK: test_proc"
Write-Host ""
Write-Host "Deploy no emulador:"
Write-Host '  $ADB push test_proc /data/local/tmp/'
Write-Host '  $ADB shell chmod 755 /data/local/tmp/test_proc'
Write-Host '  $ADB shell "su -c /data/local/tmp/test_proc com.dts.freefireth libil2cpp.so"'
