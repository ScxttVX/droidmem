@echo off
REM build_module.bat — Compilar droidmem.ko para emulador (kernel 4.19.195)
REM Uso: build_module.bat [caminho_kernel_src]

setlocal

set MODULE_DIR=X:\hackings\droidmem\droidmem\km
set KERNEL_SRC=%~1

if "%KERNEL_SRC%"=="" (
    echo ===================================================
    echo  droidmem - Compilador de Modulo Kernel
    echo ===================================================
    echo.
    echo Kernel do emulador: 4.19.195 (x86_64)
    echo.
    echo Para compilar, primeiro obter o kernel source:
    echo   git clone --depth=1 --branch android-4.19-common
    echo   https://android.googlesource.com/kernel/common kernel_src
    echo.
    echo Depois executar:
    echo   build_module.bat X:\caminho\kernel_src
    echo.
    goto :end
)

echo [*] Compilando modulo kernel com: %KERNEL_SRC%

where wsl >nul 2>&1
if %ERRORLEVEL% equ 0 (
    echo [*] Using WSL...
    wsl bash -c "make -C '%KERNEL_SRC%' M='%MODULE_DIR%' ARCH=x86_64 modules"
    goto :check
)

where make >nul 2>&1
if %ERRORLEVEL% equ 0 (
    echo [*] Using make...
    make -C "%KERNEL_SRC%" M="%MODULE_DIR%" ARCH=x86_64 modules
    goto :check
)

echo [-] Necessario WSL ou make. Instalar: wsl --install
goto :end

:check
if exist "%MODULE_DIR%\droidmem.ko" (
    echo [+] Build OK: %MODULE_DIR%\droidmem.ko
    echo.
    echo Proximos passos:
    echo   adb push %MODULE_DIR%\droidmem.ko /data/local/tmp/
    echo   adb shell su -c "insmod /data/local/tmp/droidmem.ko"
    echo   adb shell su -c "chmod 666 /dev/droidmem"
    echo   adb shell /data/local/tmp/main64 com.dts.freefireth libil2cpp.so
) else (
    echo [-] Falha na compilacao
)

:end
endlocal
