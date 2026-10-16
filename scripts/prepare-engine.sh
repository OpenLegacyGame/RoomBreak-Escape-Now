#!/usr/bin/env bash
set -euo pipefail
project_dir=$(cd "$(dirname "$0")/.." && pwd)
engine_dir=${1:-${AX_ROOT:-"$project_dir/external/axmol"}}
if [[ ! -e "$engine_dir/.git" ]]; then
  git clone --depth 1 --branch v2.11.5 https://github.com/axmolengine/axmol.git "$engine_dir"
fi
expected=6c0dce6db4660feafe29dabb1ab3506ee76b0931
[[ $(git -C "$engine_dir" rev-parse HEAD) == "$expected" ]] || { echo 'Expected Axmol v2.11.5 source revision' >&2; exit 1; }
patch_file="$project_dir/scripts/patches/axmol-2.11.5-modern-toolchains.patch"
if git -C "$engine_dir" apply --reverse --check "$patch_file" 2>/dev/null; then
  echo 'Engine compatibility patch already present.'
else
  git -C "$engine_dir" apply --check "$patch_file"
  git -C "$engine_dir" apply "$patch_file"
fi
printf 'AX_ROOT=%s\n' "$engine_dir"
