#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
  echo "Usage: $0 \"<test_case_regex>\" [extra_ctest_args...]"
  exit 2
fi

test_case_regex="$1"
shift

if ! repo_root="$(git rev-parse --show-toplevel 2>/dev/null)"; then
  echo "Error: run this script from within a git working tree."
  exit 2
fi
cd "${repo_root}"

previous_preset="$(basename "$(readlink build/current 2>/dev/null || true)")"

restore_previous_preset() {
  if [[ -n "${previous_preset}" && "${previous_preset}" != "Host-Debug" ]]; then
    cmake --preset "${previous_preset}" >/dev/null 2>&1 || true
  fi
}
trap restore_previous_preset EXIT

cmake --preset Host-Debug
cmake --build --preset Host-Debug
ctest --preset Host-Debug -R "${test_case_regex}" --output-on-failure "$@"
