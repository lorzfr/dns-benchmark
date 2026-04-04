[CmdletBinding()]
param(
    [string]$InstallDir = "$env:LOCALAPPDATA\dnsbenchmark",
    [switch]$AddToUserPath
)

$ErrorActionPreference = "Stop"

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$sourceScript = Join-Path $scriptRoot "dnsbenchmark.ps1"

if (-not (Test-Path $sourceScript)) {
    throw "dnsbenchmark.ps1 was not found next to install-windows.ps1"
}

New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null
$targetScript = Join-Path $InstallDir "dnsbenchmark.ps1"
Copy-Item -Path $sourceScript -Destination $targetScript -Force

$shimPath = Join-Path $InstallDir "dnsbenchmark.cmd"
$shimContent = "@echo off`r`n" +
    "powershell.exe -NoProfile -ExecutionPolicy Bypass -File `"%~dp0dnsbenchmark.ps1`" %*`r`n"
Set-Content -Path $shimPath -Value $shimContent -Encoding Ascii

if ($AddToUserPath) {
    $currentPath = [Environment]::GetEnvironmentVariable("Path", "User")
    $parts = @()

    if ($currentPath) {
        $parts = $currentPath -split ';'
    }

    if ($parts -notcontains $InstallDir) {
        $newPath = if ($currentPath) { "$currentPath;$InstallDir" } else { $InstallDir }
        [Environment]::SetEnvironmentVariable("Path", $newPath, "User")
        Write-Host "Added '$InstallDir' to your User PATH. Open a new shell to use it."
    }
    else {
        Write-Host "User PATH already contains '$InstallDir'."
    }
}

Write-Host "Installed dnsbenchmark to: $InstallDir"
Write-Host "Run with: `"$shimPath`" --help"
if (-not $AddToUserPath) {
    Write-Host "Tip: re-run with -AddToUserPath to make 'dnsbenchmark' available from any new shell."
}
