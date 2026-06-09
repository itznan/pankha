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
├── build.ps1                  # PowerShell compilation script (C# + C++)
└── build.bat                  # Batch build script wrapper
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
You can compile the entire application (including copying DLLs and generating the installer) by running the PowerShell build script:

```powershell
# Run the PowerShell build script
powershell -ExecutionPolicy Bypass -File build.ps1
```

The compiled distribution folder will be outputted under `dist/` and the installer will be generated under `installer/pankha_installer.exe`.

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
