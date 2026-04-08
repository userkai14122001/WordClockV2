param(
    [switch]$RunBuild
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot

Write-Host "[smoke] Checking version consistency..."
$pio = Get-Content "platformio.ini" -Raw
$manifest = Get-Content "ota_manifest.json" -Raw | ConvertFrom-Json

$matches = [regex]::Matches($pio, 'FIRMWARE_VERSION=\\"([^\\"]+)\\"')
if ($matches.Count -lt 1) {
    throw "No FIRMWARE_VERSION entries found in platformio.ini"
}

$versions = @($matches | ForEach-Object { $_.Groups[1].Value } | Select-Object -Unique)
if ($versions.Count -ne 1) {
    throw "Multiple firmware versions found in platformio.ini: $($versions -join ', ')"
}

$fwVersion = $versions[0]
if ($manifest.version -ne $fwVersion) {
    throw "Version mismatch: platformio.ini=$fwVersion ota_manifest.json=$($manifest.version)"
}

Write-Host "[smoke] Version OK: $fwVersion"

if (-not [string]::IsNullOrWhiteSpace($manifest.firmware_url)) {
    Write-Host "[smoke] firmware_url present"
} else {
    throw "ota_manifest.json missing firmware_url"
}

if ($RunBuild) {
    Write-Host "[smoke] Running platformio build..."
    $env:PATH = "C:\Users\uif41789\.platformio\penv\Scripts;$env:PATH"
    platformio run
}

Write-Host "[smoke] PASS"
