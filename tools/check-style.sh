#!/bin/bash
# check-style.sh — reports files that differ from clang-format expectations
# Usage: ./check-style.sh [--fix] [file ...]
#   --fix   apply formatting in-place instead of just reporting
#   file    one or more specific files to check (defaults to all .cpp/.h)
#
# Note: run --fix on new or modified files to bring them into compliance.
# Some existing files contain hand-aligned code that predates this style guide.

SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
CLANG_FORMAT="${CLANG_FORMAT:-$(command -v clang-format-19 || command -v clang-format)}"
FIX=0
FILES=()

for arg in "$@"; do
  if [[ "$arg" == "--fix" ]]; then
    FIX=1
  else
    FILES+=("$arg")
  fi
done

if [[ ${#FILES[@]} -eq 0 ]]; then
  mapfile -t FILES < <(find "$SCRIPT_DIR" \( -name "*.cpp" -o -name "*.h" \) \
    ! -path "*/QRetro/*" | sort)
fi

FAIL=0

for f in "${FILES[@]}"; do
  if [[ "$FIX" -eq 1 ]]; then
    "$CLANG_FORMAT" -i "$f"
    echo "formatted: $f"
  else
    DIFF=$(diff --unified=2 <("$CLANG_FORMAT" "$f") "$f")
    if [[ -n "$DIFF" ]]; then
      echo "FAIL: $f"
      echo "$DIFF" | grep -E "^(@@|\+[^+]|-[^-])" | sed 's/^/  /'
      echo ""
      FAIL=1
    fi
  fi
done

if [[ "$FIX" -eq 0 ]]; then
  if [[ $FAIL -eq 0 ]]; then
    echo "All files pass style check."
  else
    echo ""
    echo "Run with --fix to apply formatting automatically."
    exit 1
  fi
fi
