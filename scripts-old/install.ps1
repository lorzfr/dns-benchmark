[CmdletBinding()]
param(
    [string]$InstallDir = "$env:LOCALAPPDATA\dnsbenchmark",
    [switch]$AddToUserPath,
    [string]$RepoUrl = "https://github.com/lorz/dns-benchmark.git",
    [string]$RepoRef = "main"
)

$ErrorActionPreference = "Stop"

function Get-SourceScriptPath {
    $scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
    if ($scriptRoot -and (Test-Path (Join-Path $scriptRoot "dnsbenchmark.ps1"))) {
        return (Join-Path $scriptRoot "dnsbenchmark.ps1")
    }

    $tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("dnsbenchmark-install-" + [guid]::NewGuid().ToString("N"))
    git clone --depth 1 --branch $RepoRef $RepoUrl $tempRoot | Out-Null
    return (Join-Path $tempRoot "dnsbenchmark.ps1")
}

$sourceScript = Get-SourceScriptPath
if (-not (Test-Path $sourceScript)) {
    throw "Could not locate dnsbenchmark.ps1 in the source."
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
}

Write-Host "Installed dnsbenchmark to: $InstallDir"
Write-Host "Run with: `"$shimPath`" --help"
