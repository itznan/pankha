# Building Pankha

This document provides complete, step-by-step instructions for setting up your development environment, compiling Pankha from source, and packaging the Windows installer.

---

## Prerequisites

To compile and package Pankha, your development machine must meet the following requirements:

### 1. Operating System
* Windows 10 (Build 19041 or higher) or Windows 11.
* Administrator access is required during run time and installer compilation due to kernel-level hardware driver registration.

### 2. .NET 10.0 SDK
* Pankha is built on .NET 10.0 and Avalonia UI. 
* You can install the .NET 10.0 SDK globally, or let the automated build script download a localized copy into the project folder.

### 3. Inno Setup 6 (Optional)
* Required if you want to compile the single-file setup installer (`pankha_installer.exe`).
* Download and install from the official site or install via Chocolatey:
  ```powershell
  choco install innosetup -y
  ```

---

## Directory Layout Overview

Before building, it is helpful to understand the source structure:

```
pankha/
├── src/                    # C# Source Code & UI Layouts
│   ├── Assets/             # Graphical assets (logo, etc.)
│   ├── Components/         # Telemetry & Curve Charts
│   ├── Services/           # Core API wrappers (LibreHardwareMonitor & PawnIO)
│   ├── ViewModels/         # MVVM Presentation Logic
│   ├── Views/              # Avalonia XAML Windows
│   ├── app.manifest        # Windows UAC Elevation Settings
│   └── pankha.csproj       # Project & Version Configuration
├── installer/
│   └── setup.iss           # Inno Setup compilation instructions
├── docs/
│   └── BUILDING.md         # This document
├── build.bat               # Automated build pipeline
└── README.md               # Main Project documentation
```

---

## Build Methods

You can build Pankha using any of the three methods below.

### Method A: Automated Build Script (Recommended)

The project includes `build.bat` in the root directory. This script performs the following tasks:
1. Checks for a global .NET 10.0 SDK. If not found, it downloads a portable .NET 10.0 SDK into a local `.dotnet/` folder.
2. Cleans any previous builds.
3. Publishes a standalone, self-contained `win-x64` executable to the `dist/` folder.
4. Searches for Inno Setup 6 and compiles the final setup installer package.

To run it:
1. Open PowerShell or Command Prompt.
2. Navigate to the project root directory.
3. Run the script:
   ```powershell
   .\build.bat
   ```

Upon completion, you will find the standalone binaries in the `dist/` directory and the installer at `installer/pankha_installer.exe`.

### Method B: Manual CLI Build

If you prefer to compile manually using the .NET CLI:

1. Restore dependencies and compile the self-contained execution package:
   ```powershell
   dotnet publish src/pankha.csproj -c Release -r win-x64 --self-contained true
   ```
2. Clean up or create the distribution directory:
   ```powershell
   if (Test-Path dist) { Remove-Item -Path "dist\*" -Recurse -Force }
   New-Item -ItemType Directory -Path "dist" -Force
   ```
3. Copy the compiled binaries to the distribution folder:
   ```powershell
   Copy-Item -Path "src\bin\Release\net10.0-windows\win-x64\publish\FanControlHost.exe" -Destination "dist\" -Force
   ```
4. Compile the installer with Inno Setup (replace path with your Inno Setup installation path if different):
   ```powershell
   & "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" installer\setup.iss
   ```

### Method C: IDE (Visual Studio / JetBrains Rider)

1. Open your IDE of choice (Visual Studio 2022/2025 or JetBrains Rider).
2. Open the project file: `src/pankha.csproj`.
3. Set the build configuration to `Release` and target platform to `x64`.
4. Build the solution.
5. To publish a single self-contained file, right-click the project, select **Publish**, and choose the profile settings matching `win-x64`, `Self-contained`, and `Single file`.

---

## Kernel Driver Dependency (PawnIO)

LibreHardwareMonitorLib interacts with hardware register levels using a kernel-mode driver interface. Pankha utilizes the **PawnIO** kernel driver service.

* **Automatic Setup**: When Pankha starts up, it automatically queries the Windows Registry key `SYSTEM\CurrentControlSet\Services\PawnIO`. If the service is missing, it displays an installation wizard (`PawnIOInstallWindow.axaml`), downloads the driver setup utility from the official repository (`namazso/PawnIO.Setup`), installs it silently, and requests a reboot if necessary.
* **Manual Setup**: If you prefer to install it yourself beforehand, you can download `PawnIO.Setup.exe` directly from the [PawnIO Releases on GitHub](https://github.com/namazso/PawnIO.Setup/releases) and run it.

---

## Troubleshooting

### 1. Error: NETSDK1045 - Current .NET SDK does not support targeting .NET 10.0
* **Cause**: Your system's global `dotnet` command is running an older .NET SDK version (such as .NET 8.0).
* **Solution**: Run `.\build.bat` directly. It will automatically install and use a localized instance of the .NET 10.0 SDK without modifying your system's global .NET settings.

### 2. Inno Setup Compiler Not Found
* **Cause**: Inno Setup is not installed, or is installed in a non-standard directory.
* **Solution**: The build script checks the standard paths. If you installed it in a custom directory, open `build.bat` in a text editor and update the path variable:
  ```batch
  set "ISCC_PATH=C:\Path\To\Your\Inno Setup 6\ISCC.exe"
  ```

### 3. Application Crashes on Startup (No Hardware Access)
* **Cause**: The application was launched without administrative privileges, or Windows Defender Blocked the Super I/O driver wrapper.
* **Solution**: Check that the application is running elevated. Run `FanControlHost.exe` as Administrator.
