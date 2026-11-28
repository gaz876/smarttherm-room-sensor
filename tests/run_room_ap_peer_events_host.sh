#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
w=$(mktemp -d)
trap 'rm -rf "$w"' EXIT HUP INT TERM
# Copy exact header to keep host mock dependencies separate from real SDK siblings.
cp src/room_apsta_events.h "$w/room_apsta_events.h"
cmp src/room_apsta_events.h "$w/room_apsta_events.h"
cp main/room_wifi_apsta_link.h "$w/room_wifi_apsta_link.h"
cmp main/room_wifi_apsta_link.h "$w/room_wifi_apsta_link.h"
cp src/room_http_peer_guard.h "$w/room_http_peer_guard.h"
cmp src/room_http_peer_guard.h "$w/room_http_peer_guard.h"
cp main/room_http_idf_peer.h "$w/room_http_idf_peer.h"
cmp main/room_http_idf_peer.h "$w/room_http_idf_peer.h"
cp src/room_cleanup_budget.h "$w/room_cleanup_budget.h"
cmp src/room_cleanup_budget.h "$w/room_cleanup_budget.h"
for f in room_apsta_events.c room_wifi_apsta_link.c room_http_idf_peer.c; do
 cp "main/$f" "$w/$f"
 cmp "main/$f" "$w/$f"
done
for cc in gcc clang; do
 "$cc" --version | head -n 1
 "$cc" -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -no-pie -pthread -I"$w" -Itests/mock_ap_peer "$w/room_apsta_events.c" tests/test_room_ap_peer_events.c -o "$w/test"
 ASAN_OPTIONS=detect_leaks=1 timeout 10s "$w/test"
 printf 'exit=0 %s event-source\n' "$cc"
 "$cc" -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -no-pie -pthread -I"$w" -Itests/mock_ap_peer "$w/room_apsta_events.c" "$w/room_wifi_apsta_link.c" "$w/room_http_idf_peer.c" tests/test_room_ap_peer_link.c -o "$w/link"
 ASAN_OPTIONS=detect_leaks=1 timeout 10s "$w/link"
 printf 'exit=0 %s link\n' "$cc"
done
