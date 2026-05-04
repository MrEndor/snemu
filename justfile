set shell := ["bash", "-euo", "pipefail", "-c"]

# Default: list grouped recipes.
default:
    @just --list --unsorted

# Configure CMake for the given preset (default: debug).
[group('build')]
configure preset="debug":
    cmake --preset {{ preset }}

# Build the given preset. Assumes `just configure` has already run.
[group('build')]
build preset="debug":
    cmake --build build/{{ preset }} --parallel

# Run ctest against the given preset.
[group('build')]
test preset="debug":
    ctest --test-dir build/{{ preset }} --output-on-failure

# Remove all CMake build trees. Leaves dpdk-install/ alone (it's cached).
[group('build')]
clean:
    rm -rf build

# Configure -> build -> test against debug. Mirrors the build-test CI job.
[group('check')]
check:
    cmake --workflow --preset test

# Apply clang-format in place across src/ include/ tests/.
[group('quality')]
fmt: (_clang-format "fix")

# Read-only clang-format check across src/ include/ tests/.
[group('quality')]
fmt-check: (_clang-format "check")

# Run scripts/check-style.sh (clang-format + clang-tidy). Needs a build tree.
[group('quality')]
lint build_dir="build/debug":
    SNEMU_BUILD_DIR={{ build_dir }} scripts/check-style.sh

# Print the DPDK submodule SHA. Used as a cache key.
[group('meta')]
dpdk-sha:
    @git -C third_party/dpdk rev-parse HEAD

# clang-format dispatcher: mode is "fix" (rewrite) or "check" (dry-run).
[private]
_clang-format mode:
    #!/usr/bin/env bash
    set -euo pipefail

    mapfile -t files < <(
      find src include tests \
        \( -name '*.c' -o -name '*.h' -o -name '*.cc' \) \
        -print | sort)

    case "{{ mode }}" in
      fix)   clang-format -i "${files[@]}" ;;
      check) clang-format --dry-run --Werror "${files[@]}" ;;
      *)     echo "_clang-format: unknown mode '{{ mode }}'" >&2; exit 2 ;;
    esac
