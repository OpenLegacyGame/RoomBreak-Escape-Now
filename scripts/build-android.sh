#!/usr/bin/env bash
set -euo pipefail
project_dir=$(cd "$(dirname "$0")/.." && pwd)
export AX_ROOT=${AX_ROOT:-"$project_dir/external/axmol"}
[[ -d "$AX_ROOT/core" ]] || { echo 'Run scripts/prepare-engine.sh first, or set AX_ROOT.' >&2; exit 1; }
: "${ANDROID_HOME:?Set ANDROID_HOME to an Android SDK with API 36, build-tools 36.0.0 and NDK 30.0.16248370}"
: "${JAVA_HOME:?Set JAVA_HOME to JDK 21}"
command -v pwsh >/dev/null || { echo 'PowerShell 7 is required on PATH by Axmol.' >&2; exit 1; }
"$project_dir/platforms/android/gradlew" -p "$project_dir/platforms/android" :app:assembleDebug --console=plain

mkdir -p "$project_dir/dist/android"
cp "$project_dir/platforms/android/app/build/outputs/apk/debug/app-debug.apk" "$project_dir/dist/android/RoomBreak Escape Now.apk"
