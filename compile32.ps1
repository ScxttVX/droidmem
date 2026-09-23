$NDK = "C:\Users\VX\AppData\Local\Android\Sdk\ndk\25.1.8937393\toolchains\llvm\prebuilt\windows-x86_64\bin"
$CXX = "$NDK\armv7a-linux-androideabi21-clang++.cmd"
Set-Location "X:\hackings\droidmem\droidmem\um"
Write-Host "[*] Compilando ARM32 (dispositivos 32-bit) com: $CXX"
& $CXX -g -Wall -O2 -static -o main.o -c main.cpp
if ($LASTEXITCODE -ne 0) { Write-Host "[-] Falha na compilação"; exit 1 }
& $CXX -g -Wall -O2 -static -o main32 main.o
if ($LASTEXITCODE -ne 0) { Write-Host "[-] Falha no link"; exit 1 }
Write-Host "[+] Build OK: main32"
