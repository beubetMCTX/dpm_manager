[CmdletBinding()]
param(
    [string]$BuildDirectory = "build/codex_msvc142_release",
    [string]$DependencyDirectory = "build/Desktop_Qt_6_7_3_MSVC2022_64bit-Release",
    [string]$OutputDirectory = "release/dpm_manager",
    [string]$WindeployQt = "E:/Qt/6.7.3/msvc2019_64/bin/windeployqt.exe",
    [switch]$SkipQtDeployment
)

$ErrorActionPreference = "Stop"
$repositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

function Resolve-RepositoryPath([string]$path) {
    if ([IO.Path]::IsPathRooted($path)) {
        return [IO.Path]::GetFullPath($path)
    }
    return [IO.Path]::GetFullPath((Join-Path $repositoryRoot $path))
}

$buildPath = Resolve-RepositoryPath $BuildDirectory
$dependencyPath = Resolve-RepositoryPath $DependencyDirectory
$outputPath = Resolve-RepositoryPath $OutputDirectory
$executablePath = Join-Path $buildPath "dpm_manager.exe"

if (-not (Test-Path -LiteralPath $executablePath -PathType Leaf)) {
    throw "Release executable not found: $executablePath"
}
if (-not (Test-Path -LiteralPath $dependencyPath -PathType Container)) {
    throw "Dependency directory not found: $dependencyPath"
}

New-Item -ItemType Directory -Force -Path $outputPath | Out-Null
Copy-Item -LiteralPath $executablePath -Destination (Join-Path $outputPath "dpm_manager.exe") -Force

# The output directory may have been populated by an older package that copied
# Debug runtimes. Remove only those generated DLLs before rebuilding the set.
Get-ChildItem -LiteralPath $outputPath -Recurse -Filter "*_debug.dll" -File |
    Remove-Item -Force

# Preserve the complete Release runtime DLL set. Some vcpkg libraries depend on
# other DLLs that windeployqt cannot discover, but Debug runtimes must not be
# mixed into a Release package.
Get-ChildItem -LiteralPath $dependencyPath -Filter "*.dll" -File |
    Where-Object { $_.BaseName -notlike "*_debug" } |
    Copy-Item -Destination $outputPath -Force

$runtimeDirectories = @(
    "platforms",
    "imageformats",
    "iconengines",
    "styles",
    "generic",
    "networkinformation",
    "tls"
)
foreach ($directory in $runtimeDirectories) {
    $sourceDirectory = Join-Path $dependencyPath $directory
    if (Test-Path -LiteralPath $sourceDirectory -PathType Container) {
        Copy-Item -LiteralPath $sourceDirectory -Destination $outputPath -Recurse -Force
    }
}

if (-not $SkipQtDeployment) {
    $windeployQtPath = Resolve-RepositoryPath $WindeployQt
    if (-not (Test-Path -LiteralPath $windeployQtPath -PathType Leaf)) {
        throw "windeployqt not found: $windeployQtPath"
    }

    & $windeployQtPath --release --compiler-runtime --no-translations `
        --dir $outputPath (Join-Path $outputPath "dpm_manager.exe")
    if ($LASTEXITCODE -ne 0) {
        throw "windeployqt failed with exit code $LASTEXITCODE"
    }
}

$debugDlls = Get-ChildItem -LiteralPath $outputPath -Recurse -Filter "*_debug.dll" -File
if ($debugDlls.Count -gt 0) {
    $names = ($debugDlls | ForEach-Object { $_.FullName }) -join ", "
    throw "Release package contains Debug runtime DLLs: $names"
}

$manifestPath = Join-Path $outputPath "deployment-manifest.txt"
$manifest = @(
    "Executable: dpm_manager.exe",
    "Generated: $([DateTime]::Now.ToString('s'))",
    "Files:"
)
$manifest += Get-ChildItem -LiteralPath $outputPath -Recurse -File |
    Sort-Object FullName |
    ForEach-Object {
        $relativePath = $_.FullName.Substring($outputPath.Length).TrimStart('\', '/')
        $hash = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
        "  $relativePath  $hash"
    }
$manifest | Set-Content -LiteralPath $manifestPath -Encoding UTF8

Write-Host "Release package created: $outputPath"
Write-Host "Runtime files: $((Get-ChildItem -LiteralPath $outputPath -Recurse -File).Count)"
