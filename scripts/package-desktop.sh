#!/usr/bin/env bash
set -euo pipefail
# Usage: package-desktop.sh macos APP OUTPUT.pkg
#        package-desktop.sh windows RELEASE_DIR VC_REDIST.exe OUTPUT.exe
script_dir=$(cd "$(dirname "$0")" && pwd)
case "${1:-}" in
  macos)
    [[ $# == 3 ]] || exit 2
    app=$(cd "$(dirname "$2")" && pwd)/$(basename "$2")
    output=$(cd "$(dirname "$3")" && pwd)/$(basename "$3")
    stage=$(mktemp -d "${TMPDIR:-/tmp}/roombreak-package.XXXXXX")
    trap 'rm -rf "$stage"' EXIT
    mkdir "$stage/root"
    ditto "$app" "$stage/root/RoomBreak Escape Now.app"
    codesign --force --deep --sign "${MAC_SIGN_IDENTITY:--}" "$stage/root/RoomBreak Escape Now.app"
    codesign --verify --deep --strict "$stage/root/RoomBreak Escape Now.app"
    pkgbuild --analyze --root "$stage/root" "$stage/components.plist"
    /usr/libexec/PlistBuddy -c 'Set :0:BundleIsRelocatable false' "$stage/components.plist"
    pkgbuild --root "$stage/root" --component-plist "$stage/components.plist" --identifier local.roombreak.rebuilt.installer --version 1.0.2 --install-location /Applications "$output"
    ;;
  windows)
    [[ $# == 4 ]] || exit 2
    payload=$(cd "$2" && pwd)
    redist=$(cd "$(dirname "$3")" && pwd)/$(basename "$3")
    output=$(cd "$(dirname "$4")" && pwd)/$(basename "$4")
    if find "$payload" -type f \( -name 'roombreak-rebuilt-progress*' -o -name 'UserDefault.*' \) | read -r _; then
      echo 'Refusing to package user saves or settings.' >&2; exit 1
    fi
    stage=$(mktemp -d "${TMPDIR:-/tmp}/roombreak-package.XXXXXX")
    trap 'rm -rf "$stage"' EXIT
    mkdir "$stage/payload"
    cp -RL "$payload/." "$stage/payload/"
    payload="$stage/payload"
    makensis -WX "-DPACKAGE_DIR=$payload" "-DVC_REDIST=$redist" "-DOUTPUT_FILE=$output" "$script_dir/windows-installer.nsi"
    ;;
  *) echo 'Usage: package-desktop.sh macos APP OUTPUT.pkg | windows RELEASE_DIR VC_REDIST.exe OUTPUT.exe' >&2; exit 2 ;;
esac
