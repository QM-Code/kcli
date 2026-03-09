#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage: $0 <demo-binary> <case>"
    echo "Cases:"
    echo "  unknown_alpha_option"
    echo "  unknown_beta_option"
    echo "  unknown_renamed_option"
    echo "  known_alpha_option"
    echo "  unknown_app_option"
    echo "  known_and_unknown_option"
    echo "  alpha_alias_option"
    echo "  positional_args"
    echo "  double_dash_not_separator"
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
        ;;
    unknown_beta_option)
        run_and_split --beta-z
        if [[ "$status" -eq 0 ]]; then
            echo "Expected non-zero exit status for unknown beta option" >&2
            exit 1
        fi
        require_contains "$output" "unknown option --beta-z (use --beta to list options)"
        ;;
    unknown_renamed_option)
        run_and_split --renamed-wut
        if [[ "$status" -eq 0 ]]; then
            echo "Expected non-zero exit status for unknown renamed option" >&2
            exit 1
        fi
        require_contains "$output" "unknown option --renamed-wut (use --renamed to list options)"
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
    unknown_app_option)
        run_and_split --bogus
        if [[ "$status" -eq 0 ]]; then
            echo "Expected non-zero exit status for unknown app option" >&2
            exit 1
        fi
        require_contains "$output" "CLI error: unknown option --bogus"
        ;;
    known_and_unknown_option)
        run_and_split --alpha-message hello --bogus
        if [[ "$status" -eq 0 ]]; then
            echo "Expected non-zero exit status when mixing known and unknown options" >&2
            exit 1
        fi
        require_contains "$output" "Processing --alpha-message with value \"hello\""
        require_contains "$output" "CLI error: unknown option --bogus"
        ;;
    alpha_alias_option)
        run_and_split -a
        if [[ "$status" -ne 0 ]]; then
            echo "Expected zero exit status for alpha alias option" >&2
            echo "--- output begin ---" >&2
            echo "$output" >&2
            echo "--- output end ---" >&2
            exit 1
        fi
        require_contains "$output" "Processing --alpha-enable"
        require_not_contains "$output" "CLI error:"
        ;;
    positional_args)
        run_and_split input-a input-b
        if [[ "$status" -ne 0 ]]; then
            echo "Expected zero exit status for positional arguments" >&2
            echo "--- output begin ---" >&2
            echo "$output" >&2
            echo "--- output end ---" >&2
            exit 1
        fi
        require_not_contains "$output" "CLI error:"
        ;;
    double_dash_not_separator)
        run_and_split -- --alpha-message hello
        if [[ "$status" -eq 0 ]]; then
            echo "Expected non-zero exit status when passing '--'" >&2
            exit 1
        fi
        require_contains "$output" "Processing --alpha-message with value \"hello\""
        require_contains "$output" "CLI error: unknown option --"
        ;;
    *)
        echo "Error: unknown case '$test_case'" >&2
        usage
        exit 2
        ;;
esac

echo "PASS: $test_case"
