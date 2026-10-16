param([string]$EngineDirectory = $env:AX_ROOT)
$ErrorActionPreference = 'Stop'
$ProjectDirectory = (Resolve-Path "$PSScriptRoot/..").Path
if (-not $EngineDirectory) { $EngineDirectory = "$ProjectDirectory/external/axmol" }
if (-not (Test-Path "$EngineDirectory/.git")) {
    git clone --depth 1 --branch v2.11.5 https://github.com/axmolengine/axmol.git $EngineDirectory
    if ($LASTEXITCODE) { throw 'Engine clone failed' }
}
$ExpectedRevision = '6c0dce6db4660feafe29dabb1ab3506ee76b0931'
$ActualRevision = git -C $EngineDirectory rev-parse HEAD
if ($LASTEXITCODE -or $ActualRevision -ne $ExpectedRevision) { throw 'Expected Axmol v2.11.5 source revision' }
$PatchFile = "$ProjectDirectory/scripts/patches/axmol-2.11.5-modern-toolchains.patch"
git -C $EngineDirectory apply --reverse --check $PatchFile 2>$null
if ($LASTEXITCODE -eq 0) {
    Write-Output 'Engine compatibility patch already present.'
} else {
    git -C $EngineDirectory apply --check $PatchFile
    if ($LASTEXITCODE) { throw 'Engine compatibility patch check failed' }
    git -C $EngineDirectory apply $PatchFile
    if ($LASTEXITCODE) { throw 'Engine compatibility patch failed' }
}
Write-Output "AX_ROOT=$EngineDirectory"
