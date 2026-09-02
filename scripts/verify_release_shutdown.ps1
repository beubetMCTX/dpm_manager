[CmdletBinding()]
param(
    [string]$Executable = "release/dpm_manager/dpm_manager.exe",
    [int]$StartupTimeoutSeconds = 15,
    [int]$ShutdownTimeoutSeconds = 15,
    [string]$LogDirectory = ""
)

$ErrorActionPreference = "Stop"
$repositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

function Resolve-RepositoryPath([string]$path) {
    if ([IO.Path]::IsPathRooted($path)) {
        return [IO.Path]::GetFullPath($path)
    }
    return [IO.Path]::GetFullPath((Join-Path $repositoryRoot $path))
}

$executablePath = Resolve-RepositoryPath $Executable
if (-not (Test-Path -LiteralPath $executablePath -PathType Leaf)) {
    throw "Release executable not found: $executablePath"
}

if ([string]::IsNullOrWhiteSpace($LogDirectory)) {
    $LogDirectory = Join-Path (Split-Path -Parent $executablePath) "logs"
}
else {
    $LogDirectory = Resolve-RepositoryPath $LogDirectory
}

Add-Type @"
using System;
using System.Runtime.InteropServices;

public static class ReleaseShutdownProbe
{
    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern bool PostMessage(IntPtr hWnd, UInt32 message,
                                          IntPtr wParam, IntPtr lParam);
}
"@

$process = Start-Process -FilePath $executablePath `
    -WorkingDirectory (Split-Path -Parent $executablePath) `
    -PassThru

try {
    $deadline = (Get-Date).AddSeconds($StartupTimeoutSeconds)
    do {
        Start-Sleep -Milliseconds 250
        $process.Refresh()
        if ($process.HasExited) {
            throw "Release executable exited during startup with code $($process.ExitCode)."
        }
    } while ($process.MainWindowHandle -eq 0 -and (Get-Date) -lt $deadline)

    if ($process.MainWindowHandle -eq 0) {
        throw "Release executable did not create a main window within $StartupTimeoutSeconds seconds."
    }

    $wmClose = [UInt32]0x0010
    if (-not [ReleaseShutdownProbe]::PostMessage(
            $process.MainWindowHandle, $wmClose, [IntPtr]::Zero, [IntPtr]::Zero)) {
        throw "Unable to send WM_CLOSE to the Release main window."
    }

    if (-not $process.WaitForExit($ShutdownTimeoutSeconds * 1000)) {
        throw "Release executable did not exit within $ShutdownTimeoutSeconds seconds."
    }

    if ($process.ExitCode -ne 0) {
        throw "Release executable exited with code $($process.ExitCode)."
    }

    if (Test-Path -LiteralPath $LogDirectory -PathType Container) {
        $latestLog = Get-ChildItem -LiteralPath $LogDirectory -Filter "runtime_debug_*.log" -File |
            Sort-Object LastWriteTime -Descending |
            Select-Object -First 1
        if ($null -ne $latestLog) {
            $runtimeFaults = Select-String -Path $latestLog.FullName `
                -Pattern "\[(ERROR|FATAL)\]|QMainWindow::saveState\(\)" `
                -AllMatches
            if ($null -ne $runtimeFaults) {
                throw "Release runtime log contains an error or layout warning: $($latestLog.FullName)"
            }
            Write-Host "Release runtime log check passed: $($latestLog.Name)"
        }
    }

    Write-Host "Release startup/shutdown check passed (exit code 0)."
}
finally {
    if (-not $process.HasExited) {
        Stop-Process -Id $process.Id -Force
    }
    $process.Dispose()
}
