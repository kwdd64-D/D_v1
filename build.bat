@echo off
setlocal

:: --- EDIT THIS ONE LINE for your machine -------------------------------
:: Point it at the INCLUDE folder inside your own FASM install.
:: This is the only machine-specific setting in the whole project -
:: it never gets baked into workspace.asm.
set FASM_INCLUDE=C:\fasm\INCLUDE
:: -------------------------------------------------------------------------

set INCLUDE=%FASM_INCLUDE%

where fasm >nul 2>nul
if errorlevel 1 (
    echo [build.bat] "fasm" was not found on PATH.
    echo [build.bat] Add your FASM folder to PATH, or edit this script to call it by full path.
    exit /b 1
)

if not exist "%~dp0build" mkdir "%~dp0build"

fasm "%~dp0src\workspace.asm" "%~dp0build\workspace.exe"
if errorlevel 1 (
    echo [build.bat] Build failed.
    exit /b 1
)

echo [build.bat] Build succeeded: build\workspace.exe
endlocal
