#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
for cc in gcc clang; do
    "$cc" --version | head -n 1
    for t in test_room_settings test_room_nvs_adapter; do
        "$cc" -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=undefined -fno-sanitize-recover=all -Itests/mock_nvs -Imain -Isrc "tests/$t.c" -o "$work/$t"
        timeout 10s "$work/$t"
        printf 'exit=0 %s %s\n' "$cc" "$t"
    done
    "$cc" -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=undefined -fno-sanitize-recover=all -Itests/mock_runtime -Itests/mock_nvs -Imain -Isrc main/main.c src/room_measure.c src/sht40.c tests/test_room_settings_runtime.c -lm -o "$work/test_room_settings_runtime"
    timeout 10s "$work/test_room_settings_runtime"
    printf 'exit=0 %s %s\n' "$cc" test_room_settings_runtime
done
