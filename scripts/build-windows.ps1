param([string]$BuildDirectory = "$PSScriptRoot/../build/windows-release")
$ErrorActionPreference = 'Stop'
$ProjectDirectory = (Resolve-Path "$PSScriptRoot/..").Path
if (-not $env:AX_ROOT) { $env:AX_ROOT = "$ProjectDirectory/external/axmol" }
if (-not (Test-Path "$env:AX_ROOT/core")) { throw 'Prepare the patched Axmol v2.11.5 source and set AX_ROOT.' }
cmake -S $ProjectDirectory -B $BuildDirectory -G Ninja -DCMAKE_BUILD_TYPE=Release -DRB_VISUAL_PROBE=OFF
if ($LASTEXITCODE) { throw 'CMake configuration failed' }
cmake --build $BuildDirectory --parallel
if ($LASTEXITCODE) { throw 'Build failed' }
