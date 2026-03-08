#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage: $0 <demo-binary> <case>"
    echo "Cases:"
    echo "  unknown_alpha_option"
    echo "  known_alpha_option"
    echo "  output_option"
}

if [[ $# -ne 2 ]]; then
    usage
    exit 2
fi

binary="$1"
test_case="$2"

if [[ ! -x "$binary" ]]; then
    echo "Error: demo binary is not executable: $binary" >&2
    exit 2
fi

require_contains() {
    local output="$1"
    local needle="$2"
    if ! grep -Fq "$needle" <<<"$output"; then
        echo "Expected output to contain: $needle" >&2
        echo "--- output begin ---" >&2
        echo "$output" >&2
        echo "--- output end ---" >&2
        exit 1
    fi
}

require_not_contains() {
    local output="$1"
    local needle="$2"
    if grep -Fq "$needle" <<<"$output"; then
        echo "Expected output to not contain: $needle" >&2
        echo "--- output begin ---" >&2
        echo "$output" >&2
        echo "--- output end ---" >&2
        exit 1
    fi
}

run_case() {
    local -a args=("$@")
    local output
    set +e
    output="$("$binary" "${args[@]}" 2>&1)"
    local status=$?
    set -e
    echo "$status"$'\n'"$output"
}

run_and_split() {
    local payload
    payload="$(run_case "$@")"
    status="$(head -n1 <<<"$payload")"
    output="$(tail -n +2 <<<"$payload")"
}

status=0
output=""

case "$test_case" in
    unknown_alpha_option)
        run_and_split --alpha-d
        if [[ "$status" -eq 0 ]]; then
            echo "Expected non-zero exit status for unknown alpha option" >&2
            exit 1
        fi
        require_contains "$output" "unknown option --alpha-d (use --alpha to list options)"
        require_not_contains "$output" "KCLI demo core compile/link/integration check passed"
        ;;
    known_alpha_option)
        run_and_split --alpha-message hello
        if [[ "$status" -ne 0 ]]; then
            echo "Expected zero exit status for known alpha option" >&2
            echo "--- output begin ---" >&2
            echo "$output" >&2
            echo "--- output end ---" >&2
            exit 1
        fi
        require_contains "$output" "Processing --alpha-message with value \"hello\""
        require_not_contains "$output" "CLI error:"
        ;;
    output_option)
        run_and_split --output stdout
        if [[ "$status" -ne 0 ]]; then
            echo "Expected zero exit status for output option" >&2
            echo "--- output begin ---" >&2
            echo "$output" >&2
            echo "--- output end ---" >&2
            exit 1
        fi
        require_contains "$output" "KCLI demo core compile/link/integration check passed"
        require_not_contains "$output" "CLI error:"
        ;;
    *)
        echo "Error: unknown case '$test_case'" >&2
        usage
        exit 2
        ;;
esac

echo "PASS: $test_case"
