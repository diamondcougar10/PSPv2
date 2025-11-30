# PSPV2 Dependency Setup Script
# This script installs vcpkg and the required dependencies for PSPV2

$ErrorActionPreference = "Stop"

Write-Host "=== PSPV2 Dependency Setup ===" -ForegroundColor Cyan

# Check if vcpkg exists
$vcpkgPath = "C:\vcpkg"
$vcpkgExe = Join-Path $vcpkgPath "vcpkg.exe"

if (-not (Test-Path $vcpkgExe)) {
    Write-Host "`nvcpkg not found. Installing vcpkg to C:\vcpkg..." -ForegroundColor Yellow
    
    if (Test-Path $vcpkgPath) {
        Write-Host "Removing existing vcpkg directory..." -ForegroundColor Yellow
        Remove-Item -Path $vcpkgPath -Recurse -Force
    }
    
    Set-Location C:\
    Write-Host "Cloning vcpkg repository..." -ForegroundColor Yellow
    git clone https://github.com/Microsoft/vcpkg.git
    
    Set-Location $vcpkgPath
    Write-Host "Bootstrapping vcpkg..." -ForegroundColor Yellow
    .\bootstrap-vcpkg.bat
    
    if (-not (Test-Path $vcpkgExe)) {
        Write-Host "Failed to bootstrap vcpkg!" -ForegroundColor Red
        exit 1
    }
    
    Write-Host "vcpkg installed successfully!" -ForegroundColor Green
} else {
    Write-Host "`nvcpkg found at: $vcpkgPath" -ForegroundColor Green
}

# Install dependencies
Write-Host "`nInstalling SFML and nlohmann-json..." -ForegroundColor Yellow
Set-Location $vcpkgPath

Write-Host "Installing sfml:x64-windows..." -ForegroundColor Cyan
& $vcpkgExe install sfml:x64-windows

Write-Host "Installing nlohmann-json:x64-windows..." -ForegroundColor Cyan
& $vcpkgExe install nlohmann-json:x64-windows

# Integrate with Visual Studio
Write-Host "`nIntegrating vcpkg with Visual Studio..." -ForegroundColor Yellow
& $vcpkgExe integrate install

Write-Host "`n=== Setup Complete! ===" -ForegroundColor Green
Write-Host "`nNext steps:" -ForegroundColor Cyan
Write-Host "1. Return to your project directory: cd '$PSScriptRoot'" -ForegroundColor White
Write-Host "2. Configure CMake with vcpkg toolchain:" -ForegroundColor White
Write-Host "   cmake -S . -B build -A x64 -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake" -ForegroundColor Yellow
Write-Host "3. Build the project:" -ForegroundColor White
Write-Host "   cmake --build build --config Release" -ForegroundColor Yellow
Write-Host "4. Run the application:" -ForegroundColor White
Write-Host "   .\build\bin\Release\PSPV2.exe" -ForegroundColor Yellow
