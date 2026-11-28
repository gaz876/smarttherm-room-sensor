#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
w=$(mktemp -d)
trap 'rm -rf "$w"' EXIT HUP INT TERM
for f in room_http_backend.c room_http_backend.h room_http_idf_driver.c room_http_idf_driver.h room_http_idf_peer.c room_http_idf_peer.h room_network_owner.c room_network_owner.h room_sdk_stop.c; do cp "main/$f" "$w/$f"; cmp "main/$f" "$w/$f"; done
for cc in gcc clang; do
 "$cc" --version | head -n 1
 "$cc" -D_DEFAULT_SOURCE -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -no-pie -pthread -Itests/mock_http_backend -Isrc -I"$w" "$w/room_http_backend.c" "$w/room_http_idf_driver.c" "$w/room_http_idf_peer.c" "$w/room_network_owner.c" "$w/room_sdk_stop.c" tests/test_room_http_backend.c -o "$w/test"
 for mode in normal clean uncertain publish abort drain deadline stop-error worker idf-normal idf-name idf-start idf-register idf-peer idf-recv idf-send idf-size idf-timeout peer-guard idf-restart idf-evidence; do ASAN_OPTIONS=detect_leaks=1 timeout 10s "$w/test" "$mode"; printf 'exit=0 %s %s\n' "$cc" "$mode"; done
done
