@echo off
setlocal EnableExtensions

REM ==========================================================
REM  Eleph OS build script (Windows)
REM  Usage:
REM    build.bat          - build image
REM    build.bat run      - build and run in QEMU
REM    build.bat clean    - delete build artifacts
REM ==========================================================

set "ROOT=%~dp0"
cd /d "%ROOT%"

if /I "%~1"=="clean" goto :clean

set "CC=i686-elf-gcc"
set "LD=i686-elf-ld"
set "OBJCOPY=objcopy"
set "USE_CROSS_LD=1"

where %CC% >nul 2>nul
if errorlevel 1 (
    set "CC=gcc"
    echo [warn] i686-elf-gcc not found, fallback to gcc
)

where %LD% >nul 2>nul
if errorlevel 1 (
    set "USE_CROSS_LD=0"
    echo [warn] i686-elf-ld not found, fallback to gcc linker driver
)

where nasm >nul 2>nul
if errorlevel 1 (
    echo [error] nasm not found in PATH
    exit /b 1
)

where %OBJCOPY% >nul 2>nul
if errorlevel 1 (
    echo [error] objcopy not found in PATH
    exit /b 1
)

if not exist "build" mkdir "build"
if not exist "build\boot" mkdir "build\boot"
if not exist "build\kernel" mkdir "build\kernel"

echo [1/6] Assembling bootloader...
nasm -f bin "boot\boot.asm" -o "build\boot\boot.bin"
if errorlevel 1 goto :fail

echo [2/6] Assembling kernel entry...
if "%USE_CROSS_LD%"=="1" (
    nasm -f elf32 "boot\kernel_entry.asm" -o "build\boot\kernel_entry.o"
) else (
    nasm -f elf32 -DENTRY_UNDERSCORE=1 "boot\kernel_entry.asm" -o "build\boot\kernel_entry.o"
)
if errorlevel 1 goto :fail

echo [3/6] Compiling kernel sources...
%CC% -m32 -ffreestanding -fno-pie -fno-stack-protector -c "kernel\kernel.c" -o "build\kernel\kernel.o"
if errorlevel 1 goto :fail
%CC% -m32 -ffreestanding -fno-pie -fno-stack-protector -c "kernel\terminal.c" -o "build\kernel\terminal.o"
if errorlevel 1 goto :fail
%CC% -m32 -ffreestanding -fno-pie -fno-stack-protector -c "kernel\keyboard.c" -o "build\kernel\keyboard.o"
if errorlevel 1 goto :fail
%CC% -m32 -ffreestanding -fno-pie -fno-stack-protector -c "kernel\shell.c" -o "build\kernel\shell.o"
if errorlevel 1 goto :fail
%CC% -m32 -ffreestanding -fno-pie -fno-stack-protector -c "kernel\fs.c" -o "build\kernel\fs.o"
if errorlevel 1 goto :fail
%CC% -m32 -ffreestanding -fno-pie -fno-stack-protector -c "kernel\gfx.c" -o "build\kernel\gfx.o"
if errorlevel 1 goto :fail

echo [4/6] Linking kernel binary...
if "%USE_CROSS_LD%"=="1" (
    %LD% -m elf_i386 -Ttext 0x10000 --oformat binary -o "build\kernel.bin" ^
        "build\boot\kernel_entry.o" ^
        "build\kernel\shell.o" ^
        "build\kernel\kernel.o" ^
        "build\kernel\terminal.o" ^
        "build\kernel\keyboard.o" ^
        "build\kernel\fs.o" ^
        "build\kernel\gfx.o"
) else (
    %CC% -m32 -nostdlib -Wl,-Ttext,0x10000 -Wl,-e,_kernel_main -o "build\kernel.elf.exe" ^
        "build\boot\kernel_entry.o" ^
        "build\kernel\shell.o" ^
        "build\kernel\kernel.o" ^
        "build\kernel\terminal.o" ^
        "build\kernel\keyboard.o" ^
        "build\kernel\fs.o" ^
        "build\kernel\gfx.o"
    if errorlevel 1 goto :fail
    %OBJCOPY% -O binary "build\kernel.elf.exe" "build\kernel.bin"
)
if errorlevel 1 goto :fail

echo [5/6] Creating OS image...
if exist "build\pad.bin" del "build\pad.bin" >nul
fsutil file createnew "build\pad.bin" 65536 >nul
if errorlevel 1 goto :fail
if exist "build\os-image.bin" del "build\os-image.bin" >nul
copy /b "build\boot\boot.bin" + "build\kernel.bin" + "build\pad.bin" "build\os-image.bin" >nul
if errorlevel 1 goto :fail

echo [6/6] Build complete: build\os-image.bin

if /I "%~1"=="run" goto :run
exit /b 0

:run
where qemu-system-i386 >nul 2>nul
if errorlevel 1 (
    echo [error] qemu-system-i386 not found in PATH
    exit /b 1
)
echo Running QEMU...
qemu-system-i386 -drive format=raw,file="build\os-image.bin"
exit /b 0

:clean
echo Cleaning build artifacts...
if exist "build" rmdir /s /q "build"
echo Done.
exit /b 0

:fail
echo [error] Build failed.
exit /b 1
