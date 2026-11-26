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
