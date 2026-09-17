$ProgressPreference = 'SilentlyContinue'
$ErrorActionPreference = 'Stop'

$url = 'https://github.com/MaxSiominDev/Rakia/releases/latest/download/rakia-windows.exe'

try {
    Invoke-WebRequest -Uri $url -OutFile rakia.exe
} catch {
    Write-Host "failed to download rakia-windows.exe from the latest GitHub release: $_"
    exit 1
}

.\rakia.exe
