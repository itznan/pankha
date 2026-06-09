<p align="center">
  <img src="src/frontend/images/logo.png" alt="Pankha Logo" width="100" />
</p>

<h1 align="center">Pankha</h1>

<p align="center">
  <strong>A premium, lightweight fan control application for Windows</strong>
</p>

<p align="center">
  <a href="https://github.com/itznan/pankha/actions/workflows/release.yml">
    <img src="https://github.com/itznan/pankha/actions/workflows/release.yml/badge.svg" alt="Build Status" />
  </a>
  <a href="https://github.com/itznan/pankha/releases/latest">
    <img src="https://img.shields.io/github/v/release/itznan/pankha?color=%2310B981&label=latest" alt="Latest Release" />
  </a>
  <img src="https://img.shields.io/badge/platform-Windows%2010%2F11-blue" alt="Platform" />
  <img src="https://img.shields.io/badge/architecture-x64-orange" alt="Architecture" />
  <img src="https://img.shields.io/github/license/itznan/pankha?color=gray" alt="License" />
</p>

<p align="center">
  <em>Pankha (পাখা / पंखा) — Bengali/Hindi for "Fan"</em>
</p>

---

## ✨ Features

| Feature | Description |
|---|---|
| 🎨 **Premium UI** | macOS Sequoia-inspired Light & Dark themes with glassmorphism, smooth animations, and clean typography |
| 🌀 **Manual Fan Control** | Override any CPU, GPU, or motherboard fan channel with a target speed from 0% – 100% |
| 💾 **Persistent Speeds** | Manually set speeds remain active on hardware even after the app closes |
| 🔽 **System Tray** | Minimize to tray for unobtrusive background operation. Double-click to restore |
| 🚀 **Start on Boot** | Launch silently in the system tray when Windows starts |
| 📊 **Live Monitoring** | Real-time RPM readings, duty cycle percentages, and configurable poll intervals |
| 🏗️ **Native x64** | Both C++ and C# compile to native 64-bit binaries — no emulation layers |
| 📦 **Single Installer** | One-click Inno Setup installer bundles everything |

---

## 🏛️ Architecture

```
┌─────────────────────────────┐     HTTP (localhost:5555)     ┌──────────────────────────────┐
│                             │  ◄──── JSON requests ────►   │                              │
│    Qt C++ Frontend (UI)     │                               │    C# .NET 8.0 Backend       │
│                             │   GET  /fans                  │                              │
│  • MainWindow               │   POST /controls/{id}         │  • LibreHardwareMonitorLib   │
│  • FanCardWidget            │   POST /controls/{id}/auto    │  • HttpListener              │
│  • FanApiClient             │                               │  • FanController             │
│  • BackendLauncher          │                               │                              │
│                             │                               │  Requires: Administrator     │
└─────────────────────────────┘                               └──────────────────────────────┘
```

1. The **Qt C++ frontend** launches `FanControlBackend.exe` as a child process
2. The **C# backend** initializes LibreHardwareMonitor (requires Admin) and starts an HTTP server on port `5555`
3. The frontend polls the backend via JSON to display live sensor data and send control commands
4. Stdin monitoring ensures the backend exits cleanly when the frontend closes

---

## 📁 Project Structure

```
pankha/
├── src/
│   ├── backend/                    # C# .NET 8.0 Backend
│   │   ├── Program.cs              # HTTP server & request routing
│   │   ├── FanController.cs        # LibreHardwareMonitor interface
│   │   ├── FanControlApp.csproj    # Project configuration
│   │   └── app.manifest            # Administrator privilege manifest
│   │
│   └── frontend/                   # Qt 6 C++ Frontend
│       ├── MainWindow.h            # Main window header
│       ├── MainWindow.cpp          # Window lifecycle & events
│       ├── MainWindow_UI.cpp       # UI layout & widget setup
│       ├── MainWindow_Slots.cpp    # Signal/slot handlers
│       ├── MainWindow_Settings.cpp # Settings persistence
│       ├── FanApiClient.h/.cpp     # HTTP client for backend API
│       ├── BackendLauncher.h/.cpp  # Backend process manager
│       ├── main.cpp                # Application entry point
│       ├── components/
│       │   ├── FanCardWidget.h/.cpp      # Fan list card widget
│       │   └── SmoothScrollFilter.h/.cpp # Inertial scroll filter
│       ├── styles_dark.qss         # Dark theme stylesheet
│       ├── styles_light.qss        # Light theme stylesheet
│       ├── resources.qrc           # Qt resource configuration
│       ├── resource.rc             # Windows resource compiler
│       ├── icons/                  # SVG icons & .ico logo
│       └── images/                 # PNG logo
│
├── installer/
│   └── setup.iss                   # Inno Setup 6 installer script
│
├── .github/workflows/
│   └── release.yml                 # CI/CD: build, package & release
│
└── CMakeLists.txt                  # CMake build configuration
```

---

## 🔧 Build Instructions

### Prerequisites

| Tool | Version | Purpose |
|---|---|---|
| .NET SDK | 8.0+ | Build the C# backend |
| MSYS2 (Clang64) | Latest | C++ compiler toolchain |
| CMake | 3.16+ | Build system generator |
| Ninja | Latest | Build backend for CMake |
| Qt 6 | 6.x | UI framework (via MSYS2) |
| Inno Setup 6 | Optional | Compile the installer |

### Install MSYS2 Packages

```bash
pacman -S mingw-w64-clang-x86_64-clang \
          mingw-w64-clang-x86_64-lld \
          mingw-w64-clang-x86_64-cmake \
          mingw-w64-clang-x86_64-ninja \
          mingw-w64-clang-x86_64-qt6-base \
          mingw-w64-clang-x86_64-qt6-svg
```

### Step-by-Step Build

**1. Build the C# Backend**
```powershell
dotnet publish src/backend/FanControlApp.csproj -c Release -r win-x64 --self-contained true
```

**2. Build the Qt C++ Frontend**
```powershell
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_PREFIX_PATH=C:/msys64/clang64 `
  -DCMAKE_C_COMPILER=C:/msys64/clang64/bin/clang.exe `
  -DCMAKE_CXX_COMPILER=C:/msys64/clang64/bin/clang++.exe

cmake --build build --config Release
```

**3. Deploy Dependencies**
```powershell
New-Item -ItemType Directory -Path "dist" -Force
Copy-Item "build\FanControlHost.exe" -Destination "dist\" -Force
Copy-Item "src\backend\bin\Release\net8.0\win-x64\publish\*" -Destination "dist\" -Force
C:\msys64\clang64\bin\windeployqt.exe --release --no-translations --compiler-runtime dist\FanControlHost.exe
```

**4. Build Installer** *(optional)*
```powershell
& "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" installer\setup.iss
```

The installer is output to `installer/pankha_installer.exe`.

---

## 🚀 CI/CD

Every push to `main` automatically triggers a GitHub Actions workflow that:

1. Builds the C# backend (self-contained, x64)
2. Builds the Qt C++ frontend (MSYS2 Clang64)
3. Deploys all runtime dependencies
4. Compiles the Inno Setup installer
5. Uploads the installer as a build artifact
6. Creates a GitHub Release with the installer attached

See [`.github/workflows/release.yml`](.github/workflows/release.yml) for the full pipeline.

---

## ⚙️ Configuration

Settings are stored in the **Windows Registry**:

| Key | Location |
|---|---|
| App preferences | `HKCU\Software\itznan\Pankha` |
| Start on boot | `HKCU\Software\Microsoft\Windows\CurrentVersion\Run\PankhaFanControl` |

Settings include: theme selection, poll interval, minimize-to-tray behavior, and start-on-boot toggle.

---

## 📝 API Reference

The C# backend exposes a local REST API on `http://localhost:5555`:

| Method | Endpoint | Description |
|---|---|---|
| `GET` | `/fans` | List all detected fan sensors with RPM, duty %, and control IDs |
| `GET` | `/controls` | List all controllable fan channels |
| `POST` | `/controls/{id}` | Set manual speed: `{ "mode": "manual", "speed": 50 }` |
| `POST` | `/controls/{id}/auto` | Reset a fan channel to automatic BIOS control |

---

## 🤝 Contributing

Contributions are welcome! To get started:

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/my-feature`
3. Commit your changes: `git commit -m "feat: add my feature"`
4. Push to the branch: `git push origin feature/my-feature`
5. Open a Pull Request

---

## 📄 License

This project is open source. See the [LICENSE](LICENSE) file for details.

---

<p align="center">
  Made with ❤️ by <a href="https://github.com/itznan">itznan</a>
</p>
