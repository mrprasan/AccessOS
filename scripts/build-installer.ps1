# build-installer.ps1 — AccessOS installer build script (ACCESSOS-039)
#
# Builds the MSIX package (via dotnet publish) and optionally the WiX MSI.
#
# Usage:
#   .\scripts\build-installer.ps1               # MSIX only
#   .\scripts\build-installer.ps1 -BuildMsi     # MSIX + MSI (requires WiX v4)
#   .\scripts\build-installer.ps1 -Config Release
#
# Prerequisites:
#   - .NET SDK 8+ (dotnet)
#   - Visual Studio Build Tools 2022 with MSVC + Windows SDK
#   - WiX Toolset v4 (optional, for -BuildMsi): https://wixtoolset.org
#     Install via:  dotnet tool install --global wix

param(
    [string] $Config    = "Release",
    [string] $Platform  = "x64",
    [switch] $BuildMsi  = $false,
    [string] $OutDir    = "$PSScriptRoot\..\dist"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$Root       = Resolve-Path "$PSScriptRoot\.."
$UiDir      = "$Root\src\UI"
$InstallerDir = "$Root\installer"
$PublishDir = "$UiDir\bin\$Config\net8.0-windows10.0.26100.0\win-$Platform\publish"

Write-Host "=== AccessOS Installer Build ===" -ForegroundColor Cyan
Write-Host "Config   : $Config"
Write-Host "Platform : $Platform"
Write-Host "Output   : $OutDir"
Write-Host ""

# ── 1. Build native C++ core ─────────────────────────────────────────────────
Write-Host "--- Building C++ core ---" -ForegroundColor Yellow

$CmakePath = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if (-not (Test-Path $CmakePath)) {
    $CmakePath = "cmake"   # fallback to PATH
}

& $CmakePath --build "$Root\build" --target AccessOSCoreDll --config $Config
if ($LASTEXITCODE -ne 0) { throw "C++ core build failed" }

# Copy the release DLL to the UI publish staging area
$DllSuffix = if ($Config -eq "Debug") { "_d" } else { "" }
$CoreDll   = "$Root\build\bin\$Config\AccessOSCore$DllSuffix.dll"
if (-not (Test-Path $CoreDll)) { throw "Core DLL not found: $CoreDll" }

# ── 2. dotnet publish (self-contained, single-file friendly) ─────────────────
Write-Host ""
Write-Host "--- Publishing WinUI 3 app ---" -ForegroundColor Yellow

dotnet publish "$UiDir\AccessOS.UI.csproj" `
    -c $Config `
    -r "win-$Platform" `
    --self-contained false `
    -p:Platform=$Platform `
    -o "$PublishDir"

if ($LASTEXITCODE -ne 0) { throw "dotnet publish failed" }

# Copy core DLL into publish output (the POST_BUILD copies to bin, not publish)
Copy-Item -Force $CoreDll "$PublishDir\AccessOSCore.dll"

# ── 3. Package as MSIX ───────────────────────────────────────────────────────
Write-Host ""
Write-Host "--- Building MSIX package ---" -ForegroundColor Yellow

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$MsixOut = "$OutDir\AccessOS-$Config-$Platform.msix"

dotnet publish "$UiDir\AccessOS.UI.csproj" `
    -c $Config `
    -r "win-$Platform" `
    --self-contained false `
    -p:Platform=$Platform `
    -p:AppxPackageDir="$OutDir\" `
    -p:GenerateAppxPackageOnBuild=true

if ($LASTEXITCODE -ne 0) {
    Write-Warning "MSIX packaging step returned non-zero — check output above."
} else {
    Write-Host "MSIX written to: $OutDir" -ForegroundColor Green
}

# ── 4. Build WiX MSI (optional) ──────────────────────────────────────────────
if ($BuildMsi) {
    Write-Host ""
    Write-Host "--- Building WiX MSI ---" -ForegroundColor Yellow

    $WixExe = (Get-Command "wix" -ErrorAction SilentlyContinue)?.Source
    if (-not $WixExe) {
        Write-Warning "WiX toolset (wix) not found in PATH. Skipping MSI build."
        Write-Host "Install via: dotnet tool install --global wix"
    } else {
        $MsiOut = "$OutDir\AccessOS-Setup-$Config-$Platform.msi"
        & wix build "$InstallerDir\AccessOS.wxs" `
            -d "PUBLISH_DIR=$PublishDir" `
            -arch x64 `
            -o $MsiOut

        if ($LASTEXITCODE -ne 0) { throw "WiX MSI build failed" }
        Write-Host "MSI written to: $MsiOut" -ForegroundColor Green
    }
}

# ── Summary ───────────────────────────────────────────────────────────────────
Write-Host ""
Write-Host "=== Build complete ===" -ForegroundColor Green
Write-Host "Output directory: $OutDir"
Get-ChildItem $OutDir -Filter "AccessOS*" -ErrorAction SilentlyContinue |
    ForEach-Object { Write-Host "  $($_.Name)  ($([Math]::Round($_.Length/1KB, 1)) KB)" }
