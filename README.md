# Pankha – Fan Control Application

**Pankha** (Bengali/Hindi for "Fan") is a premium, lightweight, and modern fan control application designed for Windows systems. It features a stunning macOS Sequoia-styled UI built with Qt C++ and a robust hardware sensor query engine powered by a C# .NET Core backend via `LibreHardwareMonitorLib`.

---

## Key Features

- **Stunning UI Aesthetics**: Curated macOS Sequoia-like Light and Dark themes, with clean animations, glassmorphism elements, and responsive layouts.
- **Manual Override Controls**: Easily toggle manual control for any software-controllable CPU, GPU, or motherboard fan channel and adjust target speeds (0% - 100%).
- **Speed Persistence on Exit**: Manually set fan speeds remain active on hardware even after the application is closed or exited, rather than resetting to BIOS defaults.
- **Minimize to System Tray**: Runs unobtrusively in the background. Minimize or close the main window to hide it to the system tray, and restore it with a double click.
- **Start on Windows Boot**: System preference setting to run the application silently in the tray on Windows startup.
- **Native 64-bit (x64) Package**: The entire toolchain (C# and C++) compiles native x64 binaries, packaged via a native 64-bit Inno Setup installer.
- **Modularity**: Codebase is clean and modular, splitting UI widgets, custom scroll filters, and API clients into organized directories.

---

## Project Structure

```
pankha/
├── src/
│   ├── backend/               # C# .NET Core 8.0 Backend
│   │   ├── FanController.cs   # LibreHardwareMonitor API interface
│   │   ├── Program.cs         # HTTP Server for frontend requests
│   │   └── app.manifest       # Administrator privilege manifest
│   │
│   └── frontend/              # Qt C++ Frontend
│       ├── components/        # Extracted child widgets
│       │   ├── FanCardWidget.h / .cpp      # Individual fan list cards
│       │   └── SmoothScrollFilter.h / .cpp # Inertial scrolling event filter
│       ├── icons/             # App logo and SVG check icons
│       ├── images/            # Window application PNG logo
│       ├── MainWindow.h / .cpp             # Primary application UI & logic
│       ├── FanApiClient.h / .cpp           # QtNetwork client for backend API
│       ├── BackendLauncher.h / .cpp        # Manager for C# backend lifecycle
│       ├── main.cpp           # Main application entry point
│       ├── resources.qrc      # Qt resource compiler configuration
│       ├── resource.rc        # Windows icon and manifest compiler
│       ├── styles_dark.qss    # Theme stylesheet (Dark mode)
│       └── styles_light.qss   # Theme stylesheet (Light mode)
│
├── installer/                 # Package Installer
│   └── setup.iss              # Inno Setup 6 Script
│
├── CMakeLists.txt             # CMake building configuration
```

---

## Build Instructions

### Prerequisites
To build both the C# backend and C++ frontend, you will need:
1. **.NET SDK 8.0**: To build and publish the C# host process.
2. **MSYS2 with Clang64 / MinGW-w64**: Toolchain for compiling Qt C++ applications.
3. **CMake** (v3.16+) & **Ninja**: Build system generator and backend.
4. **Qt 6**: Loaded dynamically via CMake.
5. **Inno Setup 6** (Optional): Required if you wish to compile the single-file installer executable.

### Building
You can compile the application step-by-step using the following commands:

1. **Build C# Backend**:
   ```powershell
   dotnet publish src/backend/FanControlApp.csproj -c Release -r win-x64 --self-contained true
   ```

2. **Build Qt C++ Frontend**:
   ```powershell
   cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/msys64/clang64 -DCMAKE_C_COMPILER=C:/msys64/clang64/bin/clang.exe -DCMAKE_CXX_COMPILER=C:/msys64/clang64/bin/clang++.exe
   cmake --build build --config Release
   ```

3. **Deploy Dependencies**:
   Create a `dist` directory, copy the built executables, and run `windeployqt` to bundle the Qt libraries:
   ```powershell
   New-Item -ItemType Directory -Path "dist" -Force
   Copy-Item -Path "build\FanControlHost.exe" -Destination "dist\" -Force
   Copy-Item -Path "src\backend\bin\Release\net8.0\win-x64\publish\*" -Destination "dist\" -Force
   C:\msys64\clang64\bin\windeployqt.exe --release --no-translations --compiler-runtime dist\FanControlHost.exe
   ```

4. **Build Installer (Inno Setup)**:
   ```powershell
   & "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" installer\setup.iss
   ```

The generated single-file installer will be outputted to `installer/pankha_installer.exe`.

---

## How It Works

1. On startup, the Qt C++ frontend launches the compiled C# backend executable (`FanControlBackend.exe`) as a background worker process.
2. The C# backend initializes `LibreHardwareMonitorLib` (requiring Administrator privileges) and starts a local HTTP listener on `http://localhost:5555`.
3. The Qt C++ frontend communicates with the backend HTTP listener via JSON requests to query sensor list data (`GET /fans`) and apply speed adjustments (`POST /controls/{id}`).
4. Stdin monitoring is established between the parent process and backend to ensure the C# worker clean exits when the main UI closes. However, any software-override fan speed controls are left intact by bypassing standard device closes.

---

## Settings & Configurations

Preferences are persisted locally in the Windows Registry under the key:
`HKEY_CURRENT_USER\Software\itznan\Pankha`

Startup settings are registered under the Windows Startup run subkey:
`HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run\PankhaFanControl`
