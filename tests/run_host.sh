#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
for cc in gcc clang; do
    "$cc" --version | head -n 1
    for t in test_sht40 test_room_measure test_room_i2c_retry test_room_board test_room_settings; do
        "$cc" -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=undefined -fno-sanitize-recover=all -Isrc src/sht40.c src/room_measure.c "tests/$t.c" -lm -o "$work/$t"
        timeout 10s "$work/$t"
        printf 'exit=0 %s %s\n' "$cc" "$t"
    done
done
