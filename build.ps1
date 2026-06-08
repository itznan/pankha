# pankha Build Script
# This script builds both the C# backend and Qt C++ frontend, and compiles the Inno Setup installer.

Write-Host "===============================================" -ForegroundColor Cyan
Write-Host "Building pankha Application (by itznan)" -ForegroundColor Cyan
Write-Host "===============================================" -ForegroundColor Cyan

# 1. Build C# Backend
Write-Host "`n[1/4] Building C# Backend..." -ForegroundColor Yellow
& "E:\dotnet-sdk\dotnet.exe" publish -c Release -r win-x64 --self-contained true

if ($LASTEXITCODE -ne 0) {
    Write-Error "C# Backend compilation failed."
    exit 1
}

# 2. Build Qt C++ Frontend
Write-Host "`n[2/4] Building Qt C++ Frontend..." -ForegroundColor Yellow
$env:PATH = "C:\msys64\clang64\bin;" + $env:PATH
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/msys64/clang64
cmake --build build --config Release

if ($LASTEXITCODE -ne 0) {
    Write-Error "Qt C++ Frontend compilation failed."
    exit 1
}

# 3. Deploy Dependencies to dist/
Write-Host "`n[3/4] Organizing distribution and deploying dependencies..." -ForegroundColor Yellow
New-Item -ItemType Directory -Path "dist" -Force | Out-Null
Copy-Item -Path "build\FanControlHost.exe" -Destination "dist\" -Force
Copy-Item -Path "bin\Release\net8.0\win-x64\publish\*" -Destination "dist\" -Force
windeployqt.exe --release --no-translations --compiler-runtime dist\FanControlHost.exe

# Statically trace and copy all compiler and runtime DLL dependencies recursively
Write-Host "Resolving MSYS2 runtime dependencies recursively..." -ForegroundColor Yellow
$distDir = "E:\Github\Pankha\dist"
$msys2Bin = "C:\msys64\clang64\bin"
$objdump = "$msys2Bin\objdump.exe"

$queue = New-Object System.Collections.Queue
Get-ChildItem -Path $distDir -Recurse | Where-Object { $_.Extension -eq ".exe" -or $_.Extension -eq ".dll" } | ForEach-Object {
    $queue.Enqueue($_.FullName)
}

$processed = New-Object System.Collections.Generic.HashSet[string]

while ($queue.Count -gt 0) {
    $file = $queue.Dequeue()
    if ($processed.Contains($file)) { continue }
    $processed.Add($file) | Out-Null

    if (Test-Path $file) {
        # Scan PE header imports
        $imports = & $objdump -p $file 2>$null | Where-Object { $_ -match "DLL Name: ([^\s]+\.dll)" }
        foreach ($line in $imports) {
            if ($line -match "DLL Name: ([^\s]+\.dll)") {
                $dllName = $Matches[1]
                $sourcePath = "$msys2Bin\$dllName"
                $destPath = "$distDir\$dllName"

                if (Test-Path $sourcePath) {
                    if (!(Test-Path $destPath)) {
                        Write-Host "Auto-deploying dependency: $dllName (required by $(Split-Path $file -Leaf))" -ForegroundColor Green
                        Copy-Item $sourcePath -Destination $distDir -Force
                        $queue.Enqueue($destPath)
                    }
                }
            }
        }
    }
}

# 4. Build Installer
Write-Host "`n[4/4] Creating single-file installer..." -ForegroundColor Yellow
$isccPath = "C:\Users\lotio\AppData\Local\Programs\Inno Setup 6\ISCC.exe"
if (!(Test-Path $isccPath)) {
    $isccPath = "C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
}

if (Test-Path $isccPath) {
    & $isccPath installer\setup.iss
    if ($LASTEXITCODE -eq 0) {
        Write-Host "`n===============================================" -ForegroundColor Green
        Write-Host "SUCCESS: Installer compiled successfully!" -ForegroundColor Green
        Write-Host "Output path: E:\Github\Pankha\installer\pankha_installer.exe" -ForegroundColor Green
        Write-Host "===============================================" -ForegroundColor Green
    } else {
        Write-Error "Inno Setup compilation failed."
        exit 1
    }
} else {
    Write-Warning "Inno Setup compiler (ISCC.exe) not found. Skipping installer compilation."
    Write-Host "You can find compiled binaries in the 'dist' folder."
}
