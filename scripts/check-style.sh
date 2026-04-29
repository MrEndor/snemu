#!/usr/bin/env bash
# Verify code style: clang-format + clang-tidy across src/, include/, tests/.
# Usage: scripts/check-style.sh [--fix]
#   --fix  apply clang-format in-place (clang-tidy stays read-only)

set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

fix=0
if [[ ${1-} == "--fix" ]]; then
  fix=1
fi

mapfile -t c_sources < <(
  find src include tests \
    \( -name '*.c' -o -name '*.h' -o -name '*.cc' \) \
    -print | sort
)

if [[ ${#c_sources[@]} -eq 0 ]]; then
  echo "check-style: no source files found" >&2
  exit 1
fi

echo "==> clang-format (${#c_sources[@]} files)"
if [[ $fix -eq 1 ]]; then
  clang-format -i "${c_sources[@]}"
else
  clang-format --dry-run --Werror "${c_sources[@]}"
fi

build_dir="${SNEMU_BUILD_DIR:-build/debug-dpdk}"
if [[ ! -f "$build_dir/compile_commands.json" ]]; then
  echo "check-style: $build_dir/compile_commands.json missing" >&2
  echo "             run 'cmake --preset debug-dpdk' first," >&2
  echo "             or set SNEMU_BUILD_DIR to another build tree" >&2
  exit 1
fi

mapfile -t tidy_sources < <(
  find src -name '*.c' -print | sort
)

echo "==> clang-tidy (${#tidy_sources[@]} files, -p $build_dir)"
clang-tidy -p "$build_dir" --quiet "${tidy_sources[@]}"

echo "==> style OK"
