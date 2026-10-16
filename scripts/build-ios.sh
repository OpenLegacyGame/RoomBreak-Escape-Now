#!/usr/bin/env bash
set -euo pipefail
project_dir=$(cd "$(dirname "$0")/.." && pwd)
export AX_ROOT=${AX_ROOT:-"$project_dir/external/axmol"}
mode=${1:-simulator}
case "$mode" in
  simulator) simulator=ON ;;
  device) simulator=OFF ;;
  *) echo 'Usage: build-ios.sh [simulator|device] [build-directory]' >&2; exit 1 ;;
esac
build_dir=${2:-"$project_dir/build/ios-$mode"}
[[ -d "$AX_ROOT/core" ]] || { echo 'Run scripts/prepare-engine.sh first, or set AX_ROOT.' >&2; exit 1; }
xcodebuild -checkFirstLaunchStatus
cmake -S "$project_dir" -B "$build_dir" -G Xcode \
  -DCMAKE_TOOLCHAIN_FILE="$AX_ROOT/1k/ios.cmake" -DPLAT=iOS -DARCHS=arm64 \
  -DSIMULATOR="$simulator" -DDEPLOYMENT_TARGET=15.0 -DRB_VISUAL_PROBE=OFF
cmake --build "$build_dir" --config Release --parallel 4 -- CODE_SIGNING_ALLOWED=NO CODE_SIGNING_REQUIRED=NO
printf 'Unsigned application: %s/bin/RoomBreakEscapeNow/Release/RoomBreak Escape Now.app\n' "$build_dir"
if [[ "$mode" == device ]]; then
  echo 'For device installation, open the generated Xcode project and configure your development team and signing.'
fi
