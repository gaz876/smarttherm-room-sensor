#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
# Copy translation units so quoted SDK includes resolve to fixture mocks.
for f in room_network.c room_network_owner.c room_network_owner.h room_network.h; do
    cp "main/$f" "$work/$f"
    cmp "main/$f" "$work/$f"
done
for cc in gcc clang; do
    "$cc" --version | head -n 1
    "$cc" -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -no-pie -Itests/mock_network_owner -I"$work" "$work/room_network.c" "$work/room_network_owner.c" tests/test_room_network_owner.c -o "$work/owner"
    for mode in owner valid missing storage-error init-error station-error; do
        ASAN_OPTIONS=detect_leaks=1 timeout 10s "$work/owner" "$mode"
        printf 'exit=0 %s %s\n' "$cc" "$mode"
    done
done
