$ErrorActionPreference = "Stop"

# --- Configuration ---
$innoSetupPath = "C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
$buildConfig = "Release"
$distScript = ".\create_dist.ps1"
$issScript = ".\pspsetup.iss"

Write-Host "=== PSPV2 Installer Build Script ===" -ForegroundColor Cyan

# 1. Check for Inno Setup
if (-not (Test-Path $innoSetupPath)) {
    Write-Error "Inno Setup Compiler (ISCC.exe) not found at '$innoSetupPath'. Please install Inno Setup 6."
}

# 2. Build the Application
Write-Host "`n[1/3] Building Application ($buildConfig)..." -ForegroundColor Yellow
cmake --build build --config $buildConfig
if ($LASTEXITCODE -ne 0) { throw "Build failed." }

# 3. Create Distribution Folder
Write-Host "`n[2/3] Creating Distribution Structure..." -ForegroundColor Yellow
if (Test-Path $distScript) {
    & $distScript
} else {
    throw "Distribution script '$distScript' not found."
}

# 4. Compile Installer
Write-Host "`n[3/3] Compiling Installer..." -ForegroundColor Yellow
if (Test-Path $issScript) {
    & $innoSetupPath $issScript
    if ($LASTEXITCODE -ne 0) { throw "Inno Setup compilation failed." }
} else {
    throw "Inno Setup script '$issScript' not found."
}

Write-Host "`n=== Build Complete! ===" -ForegroundColor Green
Write-Host "Installer location: dist\PSPV2_Setup.exe" -ForegroundColor Green
