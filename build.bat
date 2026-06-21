@echo off
setlocal enabledelayedexpansion

echo =========================================
echo  Building Pankha Application (.NET 10.0)
echo =========================================

:: -------------------------------------------------------
:: Step 0: Ensure .NET 10 SDK is available
:: -------------------------------------------------------
set "LOCAL_DOTNET=%~dp0.dotnet"
set "DOTNET_CMD=dotnet"

:: Check if global dotnet has SDK 10
dotnet --list-sdks 2>nul | findstr /B "10." >nul 2>&1
if %ERRORLEVEL% equ 0 (
    echo [0/3] .NET 10 SDK found in global install.
    goto :build
)

:: Check if we have a local dotnet with SDK 10
if exist "%LOCAL_DOTNET%\dotnet.exe" (
    "%LOCAL_DOTNET%\dotnet.exe" --list-sdks 2>nul | findstr /B "10." >nul 2>&1
    if !ERRORLEVEL! equ 0 (
        echo [0/3] .NET 10 SDK found in local install.
        set "DOTNET_CMD=%LOCAL_DOTNET%\dotnet.exe"
        goto :build
    )
)

:: Download and install .NET 10 SDK locally
echo [0/3] .NET 10 SDK not found. Downloading via dotnet-install.ps1...
echo       This is a one-time setup and may take a few minutes.
echo.

if not exist "%LOCAL_DOTNET%" mkdir "%LOCAL_DOTNET%"

powershell -NoProfile -ExecutionPolicy Bypass -Command ^
    "Invoke-WebRequest -Uri 'https://dot.net/v1/dotnet-install.ps1' -OutFile '%~dp0dotnet-install.ps1'"
if %ERRORLEVEL% neq 0 (
    echo Error: Failed to download dotnet-install.ps1. Check your internet connection.
    exit /b 1
)

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0dotnet-install.ps1" ^
    -Channel "10.0" -InstallDir "%LOCAL_DOTNET%" -Version "latest"
if %ERRORLEVEL% neq 0 (
    echo Error: Failed to install .NET 10 SDK.
    exit /b 1
)

del /q "%~dp0dotnet-install.ps1" 2>nul
set "DOTNET_CMD=%LOCAL_DOTNET%\dotnet.exe"
echo .NET 10 SDK installed successfully.

:build
:: -------------------------------------------------------
:: Step 1: Build & Publish the C# App
:: -------------------------------------------------------
echo.
echo [1/3] Compiling and publishing C# application...
"%DOTNET_CMD%" publish src/pankha.csproj -c Release -r win-x64 --self-contained true
if %ERRORLEVEL% neq 0 (
    echo Error: Failed to publish the C# project.
    exit /b %ERRORLEVEL%
)

:: -------------------------------------------------------
:: Step 2: Deploy to dist folder
:: -------------------------------------------------------
echo.
echo [2/3] Preparing dist directory and copying binaries...
if exist dist (
    rd /s /q dist
)
mkdir dist

copy "src\bin\Release\net10.0-windows\win-x64\publish\FanControlHost.exe" "dist\" /y
if %ERRORLEVEL% neq 0 (
    echo Error: Failed to copy FanControlHost.exe to dist directory.
    exit /b %ERRORLEVEL%
)

:: -------------------------------------------------------
:: Step 3: Build Installer using Inno Setup
:: -------------------------------------------------------
echo.
echo [3/3] Compiling Inno Setup installer...
set "ISCC_PATH=C:\Users\lotio\AppData\Local\Programs\Inno Setup 6\ISCC.exe"

if exist "%ISCC_PATH%" (
    "%ISCC_PATH%" installer\setup.iss
    if !ERRORLEVEL! neq 0 (
        echo Error: Inno Setup compiler failed.
        exit /b !ERRORLEVEL!
    )
) else (
    echo Warning: Inno Setup compiler not found at "%ISCC_PATH%"
    echo Installer .exe was NOT generated, but binaries are in the "dist" folder.
)

echo.
echo =========================================
echo  Build Completed Successfully!
echo =========================================
pause
