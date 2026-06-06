param(
    [string]$WorkspaceRoot = "",
    [string]$CcsRoot = "D:\ccs\ccs",
    [string]$TargetConfig = "",
    [string]$OutFile = "",
    [int]$BuildTimeoutSeconds = 180,
    [int]$FlashTimeoutSeconds = 90
)

$ErrorActionPreference = "Stop"

function Quote-CommandArgument {
    param([string]$Value)

    if ($null -eq $Value) {
        return '""'
    }
    if ($Value.Length -eq 0 -or $Value.IndexOfAny([char[]]" `t`r`n`"") -ge 0) {
        return '"' + $Value.Replace('"', '\"') + '"'
    }
    return $Value
}

function Stop-ProcessTree {
    param([int]$ProcessId)

    $TaskKill = Join-Path $env:SystemRoot "System32\taskkill.exe"
    if (Test-Path -LiteralPath $TaskKill) {
        & $TaskKill /PID $ProcessId /T /F | Out-Host
        return
    }
    Stop-Process -Id $ProcessId -Force -ErrorAction SilentlyContinue
}

function Invoke-Tool {
    param(
        [string]$FilePath,
        [string[]]$Arguments,
        [string]$ArgumentLine = "",
        [string]$WorkingDirectory,
        [int]$TimeoutSeconds,
        [string]$FailureMessage
    )

    if ([string]::IsNullOrWhiteSpace($ArgumentLine)) {
        $ArgumentLine = [string]::Join(" ", ($Arguments | ForEach-Object { Quote-CommandArgument $_ }))
    }
    Write-Host "> $FilePath $ArgumentLine"

    $ProcessInfo = New-Object System.Diagnostics.ProcessStartInfo
    $ProcessInfo.FileName = $FilePath
    $ProcessInfo.Arguments = $ArgumentLine
    $ProcessInfo.UseShellExecute = $false
    if (-not [string]::IsNullOrWhiteSpace($WorkingDirectory)) {
        $ProcessInfo.WorkingDirectory = $WorkingDirectory
    }

    $Process = [System.Diagnostics.Process]::Start($ProcessInfo)
    if (-not $Process.WaitForExit($TimeoutSeconds * 1000)) {
        Write-Host "$FailureMessage timeout after $TimeoutSeconds seconds. Terminating process tree..."
        Stop-ProcessTree -ProcessId $Process.Id
        throw "$FailureMessage timeout"
    }

    if ($Process.ExitCode -ne 0) {
        throw "$FailureMessage failed, exit code: $($Process.ExitCode)"
    }
}

try {
    if ([string]::IsNullOrWhiteSpace($WorkspaceRoot)) {
        $WorkspaceRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
    }
    if ([string]::IsNullOrWhiteSpace($TargetConfig)) {
        $TargetConfig = Join-Path $WorkspaceRoot "targetConfigs\MSP430F5529.ccxml"
    }
    if ([string]::IsNullOrWhiteSpace($OutFile)) {
        $OutFile = Join-Path $WorkspaceRoot "Debug\1. LED.out"
    }

    $Gmake = Join-Path $CcsRoot "utils\bin\gmake.exe"
    $LoadTi = Join-Path $CcsRoot "ccs_base\scripting\examples\loadti\loadti.bat"
    $DebugDir = Join-Path $WorkspaceRoot "Debug"

    foreach ($Path in @($Gmake, $LoadTi, $DebugDir, $TargetConfig)) {
        if (-not (Test-Path -LiteralPath $Path)) {
            throw "Path not found: $Path"
        }
    }

    Write-Host "[1/2] CCS optimized build: $DebugDir"
    Invoke-Tool -FilePath $Gmake `
                -Arguments @("-C", $DebugDir, "all") `
                -WorkingDirectory $WorkspaceRoot `
                -TimeoutSeconds $BuildTimeoutSeconds `
                -FailureMessage "CCS build"

    if (-not (Test-Path -LiteralPath $OutFile)) {
        throw "Build output not found: $OutFile"
    }

    Write-Host "[2/2] Flash MSP430: $OutFile"
    $env:EXIT_LOADTI_WITH_ERRORLEVEL = "1"
    $LoadTiCommand = 'call "' + $LoadTi + '" -v -a "-c=' + $TargetConfig + '" "' + $OutFile + '"'
    $LoadTiArguments = '/d /s /c "' + $LoadTiCommand + '"'
    Invoke-Tool -FilePath "$env:ComSpec" `
                -ArgumentLine $LoadTiArguments `
                -WorkingDirectory $WorkspaceRoot `
                -TimeoutSeconds $FlashTimeoutSeconds `
                -FailureMessage "Flash"

    Write-Host "Done: build and flash completed."
    exit 0
} catch {
    Write-Host "ERROR: $($_.Exception.Message)"
    exit 1
}
