#!/bin/bash
# check-names.sh — reports local variables that don't use snake_case naming
# Usage: ./check-names.sh [--fix] [file ...]
#   --fix   apply renames in-place (requires compile_commands.json to be safe)
#   file    one or more specific files to check (defaults to all .cpp/.h)

SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
CLANG_TIDY="${CLANG_TIDY:-$(command -v clang-tidy-19 || command -v clang-tidy)}"

if [[ -z "$CLANG_TIDY" ]]; then
  echo "error: clang-tidy not found (install clang-tidy-19)" >&2
  exit 1
fi
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
  mapfile -t FILES < <(find "$SCRIPT_DIR" -name "*.cpp" \
    ! -name "moc_*.cpp" \
    ! -path "*/QRetro/*" | sort)
fi

CONFIG="{
  Checks: '-*,readability-identifier-naming',
  CheckOptions: [
    {key: readability-identifier-naming.LocalVariableCase, value: lower_case},
    {key: readability-identifier-naming.LocalVariableIgnoredRegexp, value: '_this|_t'},
    {key: readability-identifier-naming.ClassMemberCase, value: CamelCase},
    {key: readability-identifier-naming.ClassMemberPrefix, value: 'm_'}
  ]
}"

FAIL=0

if [[ -f "$SCRIPT_DIR/compile_commands.json" ]]; then
  TIDY_ARGS=(-p "$SCRIPT_DIR" --header-filter="$SCRIPT_DIR/.*")
else
  TIDY_ARGS=(--)
fi

for f in "${FILES[@]}"; do
  # Headers must be checked via their corresponding .cpp with --header-filter
  target="$f"
  extra_args=()
  if [[ "$f" == *.h ]]; then
    cpp="${f%.h}.cpp"
    if [[ ! -f "$cpp" ]]; then
      cpp="$SCRIPT_DIR/${f%.h}.cpp"
    fi
    if [[ ! -f "$cpp" ]]; then
      continue
    fi
    extra_args=(--header-filter="$(realpath "$f")")
    target="$cpp"
  fi

  if [[ "$FIX" -eq 1 ]]; then
    BEFORE=$(cat "$f")
    "$CLANG_TIDY" --config="$CONFIG" --fix "${TIDY_ARGS[@]}" "${extra_args[@]}" "$target" 2>/dev/null
    AFTER=$(cat "$f")
    if [[ "$BEFORE" != "$AFTER" ]]; then
      echo "fixed: $f"
    fi
  else
    OUTPUT=$("$CLANG_TIDY" --config="$CONFIG" "${TIDY_ARGS[@]}" "${extra_args[@]}" "$target" 2>/dev/null \
      | grep "warning:")
    if [[ -n "$OUTPUT" ]]; then
      echo "FAIL: $f"
      echo "$OUTPUT" | sed 's/^/  /'
      echo ""
      FAIL=1
    fi
  fi
done

if [[ "$FIX" -eq 0 ]]; then
  if [[ $FAIL -eq 0 ]]; then
    echo "All files pass naming check."
  else
    echo ""
    echo "Run with --fix to apply renames (compile_commands.json recommended)."
    exit 1
  fi
fi
