param(
    [string]$WorkspaceRoot = "",
    [string]$CcsRoot = "D:\ccs\ccs",
    [string]$TargetConfig = "",
    [string]$OutFile = ""
)

$ErrorActionPreference = "Stop"

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
& $Gmake -C $DebugDir all
if ($LASTEXITCODE -ne 0) {
    throw "CCS build failed, exit code: $LASTEXITCODE"
}

if (-not (Test-Path -LiteralPath $OutFile)) {
    throw "Build output not found: $OutFile"
}

Write-Host "[2/2] Flash MSP430: $OutFile"
& $LoadTi -v -a "-c=$TargetConfig" $OutFile
if ($LASTEXITCODE -ne 0) {
    throw "Flash failed, exit code: $LASTEXITCODE"
}

Write-Host "Done: build and flash completed."
