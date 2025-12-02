$ErrorActionPreference = "Stop"

$ffmpegUrl = "https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-win64-gpl.zip"
$zipPath = "ffmpeg.zip"
$extractPath = "ffmpeg_temp"

Write-Host "Downloading FFmpeg..."
Invoke-WebRequest -Uri $ffmpegUrl -OutFile $zipPath

Write-Host "Extracting FFmpeg..."
Expand-Archive -Path $zipPath -DestinationPath $extractPath -Force

Write-Host "Locating ffmpeg.exe..."
$ffmpegExe = Get-ChildItem -Path $extractPath -Recurse -Filter "ffmpeg.exe" | Select-Object -First 1

if ($ffmpegExe) {
    Write-Host "Found ffmpeg.exe at $($ffmpegExe.FullName)"
    Copy-Item -Path $ffmpegExe.FullName -Destination ".\ffmpeg.exe" -Force
    Write-Host "ffmpeg.exe copied to project root."
} else {
    Write-Error "Could not find ffmpeg.exe in the downloaded archive."
}

Write-Host "Cleaning up..."
Remove-Item -Path $zipPath -Force
Remove-Item -Path $extractPath -Recurse -Force

Write-Host "FFmpeg setup complete! You can now run the app."
