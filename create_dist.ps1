$ErrorActionPreference = "Stop"

$distDir = "dist"
$appName = "PSPV2"
$targetDir = "$distDir\$appName"

Write-Host "Creating distribution in $targetDir..."

# Clean up previous build
if (Test-Path $distDir) {
    Remove-Item -Path $distDir -Recurse -Force
}

New-Item -Path $targetDir -ItemType Directory -Force | Out-Null

# Copy Executable
Write-Host "Copying Executable..."
Copy-Item -Path "build/bin/Release/PSPV2.exe" -Destination $targetDir

# Copy DLLs (from build output as they are likely correct)
Write-Host "Copying DLLs..."
Get-ChildItem -Path "build/bin/Release" -Filter "*.dll" | Copy-Item -Destination $targetDir

# Copy Assets (from source to ensure latest)
Write-Host "Copying Assets..."
Copy-Item -Path "assets" -Destination $targetDir -Recurse

# Copy Config (from source to ensure latest)
Write-Host "Copying Config..."
Copy-Item -Path "config" -Destination $targetDir -Recurse

# Reset user_profile.json for distribution (Clean Install)
Write-Host "Resetting user profile for distribution..."
$defaultProfile = @{
    "first_time_setup" = $true
    "show_clock" = $true
    "show_date" = $true
    "theme" = "Background.png"
    "use_24_hour_format" = $true
    "user_name" = "User"
}
$defaultProfile | ConvertTo-Json | Set-Content "$targetDir\config\user_profile.json"

# Copy FFmpeg
Write-Host "Copying FFmpeg..."
if (Test-Path "ffmpeg.exe") {
    Copy-Item -Path "ffmpeg.exe" -Destination $targetDir
} else {
    Write-Warning "ffmpeg.exe not found in root! Audio conversion will not work."
}

# Create empty directories
Write-Host "Creating user directories..."
New-Item -Path "$targetDir\Games" -ItemType Directory -Force | Out-Null
New-Item -Path "$targetDir\Downloads" -ItemType Directory -Force | Out-Null
New-Item -Path "$targetDir\SavedData" -ItemType Directory -Force | Out-Null

# --- Dependency Handling ---
$appsDir = "$targetDir\Apps"
New-Item -Path $appsDir -ItemType Directory -Force | Out-Null

# 1. PPSSPP (Portable)
Write-Host "Downloading PPSSPP (Portable)..."
$ppssppUrl = "https://www.ppsspp.org/files/1_17_1/ppsspp_win.zip" # Using a specific version link
$ppssppZip = "ppsspp.zip"
$ppssppDest = "$appsDir\PPSSPP"

try {
    Invoke-WebRequest -Uri $ppssppUrl -OutFile $ppssppZip
    Expand-Archive -Path $ppssppZip -DestinationPath $ppssppDest -Force
    Remove-Item -Path $ppssppZip -Force
    
    # Create portable marker for PPSSPP to keep it portable
    New-Item -Path "$ppssppDest\memstick" -ItemType Directory -Force | Out-Null
    Write-Host "PPSSPP installed to $ppssppDest"
} catch {
    Write-Warning "Failed to download PPSSPP. Please manually place it in $ppssppDest"
}

# 2. VLC (Portable)
Write-Host "Setting up VLC folder..."
$vlcDest = "$appsDir\VLC"
New-Item -Path $vlcDest -ItemType Directory -Force | Out-Null
Write-Host "NOTE: Please download VLC Portable and extract it to: $vlcDest"
Write-Host "      Ensure vlc.exe is at $vlcDest\vlc.exe"

# 3. VC++ Redistributable
$redistDir = "$distDir\Redist"
New-Item -Path $redistDir -ItemType Directory -Force | Out-Null
Write-Host "Downloading VC++ Redistributable..."
$vcRedistUrl = "https://aka.ms/vs/17/release/vc_redist.x64.exe"
try {
    Invoke-WebRequest -Uri $vcRedistUrl -OutFile "$redistDir\vc_redist.x64.exe"
    Write-Host "VC++ Redistributable downloaded."
} catch {
    Write-Warning "Failed to download VC++ Redist. Please download it manually."
}

Write-Host "Distribution created successfully at $targetDir"
Write-Host "You can now zip the '$targetDir' folder or use it for the installer."
