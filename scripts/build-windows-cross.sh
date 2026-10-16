#!/usr/bin/env bash
set -euo pipefail
project_dir=$(cd "$(dirname "$0")/.." && pwd)
export AX_ROOT=${AX_ROOT:-"$project_dir/external/axmol"}
[[ -d "$AX_ROOT/core" ]] || { echo 'Run scripts/prepare-engine.sh first, or set AX_ROOT.' >&2; exit 1; }
command -v pwsh >/dev/null || { echo 'PowerShell 7 is required on PATH by Axmol.' >&2; exit 1; }
: "${WINSDK_ROOT:?Set WINSDK_ROOT to an xwin SDK/CRT directory}"
build_dir=${1:-"$project_dir/build/windows-cross"}
cmake -S "$project_dir" -B "$build_dir" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$project_dir/cmake/toolchains/windows-clang.cmake" \
  -DCMAKE_BUILD_TYPE=Release -DRB_VISUAL_PROBE=OFF
cmake --build "$build_dir" --parallel
