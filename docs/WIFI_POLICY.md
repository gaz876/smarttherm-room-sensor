# Enrollment review checkpoint — 2026-11-26

See docs/WIFI_ENROLLMENT.md for pinned SDK findings: stock manager forces FLASH storage and includes partial password debug output; do not use as-is. HMAC key generation is irreversible on real hardware. No actual credentials or AP until protected enrollment/storage verified.

---

# ESP-IDF station binding candidate — 2026-11-26

`main/room_wifi_idf.c` implements a station-only binding, no caller in main.c. The future dedicated owner task provides an authorized profile in RAM (SSID 1–32 nonzero bytes, printable WPA2/3 passphrase 8–63 bytes; 64-digit raw PSK and other auth types not supported). Wi-Fi NVS flag disabled and RAM storage selected; no credential persistence or logging by application. No AP. GPIO9 is active-low input with pull-up after boot; a 3 s gesture is reported to the future provisioning task, not acted upon. New default DHCP interface each attempt; no raw SDK IP/disconnect event accepted as authoritative; online requires live association, interface up and nonzero current IP. Stop, wait default event loop barrier, detach/destroy, deinit, second barrier before reuse. Barriers drain queued events but do not prove actual vendor producer quiescence; bench stress tests and actual concurrent/event sequencing required. Ambiguous partial init/attach/clear fails closed to avoid unsafe cleanup; not a full automatic recovery solution. No actual Wi-Fi task, network service or credentials. Direct pinned SDK source reviewed: wifi_default.c 874b3f1788d251329a9e8778748e1e2593d46171; esp_wifi.h 8c03384f53eb68f77f505a589478cb2ca007d3b2; esp_wifi_default.h d30d512e3588e738026bf7edc0b98ade2b8cd27d; esp_netif.h bb21f418595eb55ca896e4c69b6608f221cd9900. Host test command (from root):

```sh
for cc in gcc clang; do
    "$cc" -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=undefined -fno-sanitize-recover=all -Itests/mock_wifi -Isrc -Imain main/room_wifi_idf.c tests/test_room_wifi_idf.c -o "/tmp/wifi-idf-$cc" || exit 1
    for mode in normal attach start; do timeout 10s "/tmp/wifi-idf-$cc" "$mode" || exit 1; done
done
```

---

# Transport owner checkpoint — 2026-11-26

`src/room_wifi_owner.h` provides the single-owner lifecycle and BOOT gesture. Transport callbacks must return true from stop only once the previous event source is quiescent; this is a contract, not yet proven for ESP-IDF. Events have producer-bound generation IDs; deadline wins, cleanup failures block new starts, and setup success is only a trusted verified persistence event. BOOT starts disarmed and requires stable release before a 3 s hold (50 ms debounce). GPIO9 physical binding and strapping still need bench verification. Run the new test with GCC/Clang using the command below replacing test_room_wifi_policy.c with test_room_wifi_owner.c. Official SDK references reviewed for Wi-Fi station events and provisioning (v5.5.6), GPIO9 strapping and default event loop; network transport not enabled.

---

# Wi-Fi policy (experimental, not yet connected to ESP-IDF)

Single owner, monotonic millisecond clock. Policy alone performs no I/O, stores no credentials, starts no real AP, and does not alter the measurement loop. Startup with confirmed missing profile (not generic read errors) may request protected setup only if a device-local authorization mechanism is ready. Valid profile starts station; error stays off. One debounced physical BOOT gesture can request setup. Setup window 5 min, connection/IP timeout 30 s; retry 1, 2, 4, 8, 16, 32, 60 s capped indefinitely, reset after IP. These are provisional engineering defaults, not user-approved final values. No AP on loss of connectivity, even for wrong password. Setup expiry restores prior valid profile or stays off; only verified persisted configuration marks setup complete.

Adapter still required: single event/task owner; queue and validate current transport generation, discard delayed events even if state matches, stop/drain on mode changes, handle driver action failures, process deadlines before accepting late events, treat disconnect/IP loss as unavailable and wait for valid IP before service start. BOOT debounce/long press after normal boot, verify GPIO9 board/strapping. DHCP default. Keep measurements independent. No network API, provisioning transport, secret storage or real GPIO implemented here.

ESP-IDF v5.5.6 references:
- https://docs.espressif.com/projects/esp-idf/en/v5.5.6/esp32c3/api-guides/wifi.html
- https://docs.espressif.com/projects/esp-idf/en/v5.5.6/esp32c3/api-reference/provisioning/wifi_provisioning.html
- https://docs.espressif.com/projects/esp-idf/en/v5.5.6/esp32c3/api-reference/storage/nvs_bootloader.html

Provisioning candidate: protected SoftAP + authenticated Security 2 (SRP6a/AES-GCM), device-specific secret delivered locally to owner; verify Android/client UX and unique local enrollment before enabling. Never derive passwords from public MAC, never hardcode or log credentials, never open fallback AP. Test candidate network credentials in RAM, persist only after validated connection, exact readback and protected storage; preserve prior valid profile on failed setup. Review SDK manager default NVS writes and ensure no unintentional plaintext copy. Existing measurement NVS unencrypted. HMAC-backed NVS encryption requires eFuse key provisioning; no irreversible hardware operations authorized. Do not enable real credential persistence until storage and recovery design reviewed. No OTA or heating changes.

Host test from repository root:
```sh
for cc in gcc clang; do
    "$cc" -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=undefined -fno-sanitize-recover=all -Isrc tests/test_room_wifi_policy.c -o "/tmp/room_wifi_test-$cc" || exit 1
    timeout 10s "/tmp/room_wifi_test-$cc" || exit 1
done
```
