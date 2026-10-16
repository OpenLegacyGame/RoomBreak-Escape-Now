#!/usr/bin/env bash
set -euo pipefail
project_dir=$(cd "$(dirname "$0")/.." && pwd)
export AX_ROOT=${AX_ROOT:-"$project_dir/external/axmol"}
[[ -d "$AX_ROOT/core" ]] || { echo 'Run scripts/prepare-engine.sh first, or set AX_ROOT.' >&2; exit 1; }
command -v pwsh >/dev/null || { echo 'PowerShell 7 is required on PATH by Axmol.' >&2; exit 1; }
build_dir=${1:-"$project_dir/build/macos-release"}
cmake -S "$project_dir" -B "$build_dir" -G Ninja -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 -DCMAKE_BUILD_TYPE=Release -DRB_VISUAL_PROBE=OFF
cmake --build "$build_dir" --parallel
printf 'Application: %s/bin/RoomBreakEscapeNow/RoomBreak Escape Now.app\n' "$build_dir"
