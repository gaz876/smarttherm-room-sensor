# SDK and measurement audit checkpoint — 2026-11-25

Pinned source: 72fc4963fa9e489f75522a14bed36d6f4e89d872. Read all 20 tree paths and reviewed the measurement adapter, build scaffold, state machine and codec. Original codec retest actually executed in the isolated Linux environment on GCC 14.2.0 and Clang 17.0.0, both exit 0, C11 strict warnings and nonrecovering UBSan, 10 s timeout. Independently matched three Git blobs: src/sht40.c cc9dc7317de93b38ff49a5dba98af96d3c3c4385; src/sht40.h d40ace0c443e6d96e7a7eaa356e491494f330395; tests/test_sht40.c 5ff3c557559a7327c74967a903a84868483cbe5b. Command for each cc (gcc, clang): $cc -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=undefined -fno-sanitize-recover=all -Isrc src/sht40.c tests/test_sht40.c -lm -o /tmp/test_sht40; timeout 10s /tmp/test_sht40. New state-machine test was not rerun at this checkpoint; earlier evidence retained.

ESP-IDF v5.5.6 annotated tag 28871d80f730e6976c9bdc9c4cbff1c916e905a9 resolves to 880a203ae603d08a94d06aa01e0d6761958e5e9e, GitHub reports signature verified. Its tools/tools.json recommends riscv32-esp-elf esp-14.2.0_20260121, Linux x86_64 archive SHA256 b3fce4b04dd15a9f0ed1c209b5a2f389c04eff92ed85e4cc0ebc275b7d95e95a (292412032 bytes; expected manifest value, not downloaded or independently hashed). SDK, target compiler and Docker/Podman absent; DNS/HTTPS attempts for GitHub, Espressif mirror and PyPI failed. Connector can read text but cannot supply the binary toolchain. No target build or complete SDK/transitive dependency audit. No CI initiated; public CI administrative gate remains open.

Static findings: SHT4x datasheet rev 6.6 confirms 1 ms maximum reset and 8.3 ms maximum high-repeatability measurement times; 2/10 ms waits in code are conservative. ESP-IDF I2C API supports the adapter calls. GPIO4/5 remain provisional; Kconfig range 0–21 permits flash/USB/strapping pin conflicts, so constrain configuration to board-approved pins before hardware use. Bus/device initialization error currently exits app_main rather than retrying; add recoverable initialization and test it. sdkconfig.defaults sets only 4 MB flash; explicit OTA partition sizing and actual image fit remain pending, not OTA implementation. No source change, flash or heating work.

References: https://github.com/espressif/esp-idf/tree/v5.5.6 ; https://docs.espressif.com/projects/esp-idf/en/v5.5.6/esp32c3/api-reference/peripherals/i2c.html ; https://docs.espressif.com/projects/esp-idf/en/v5.5.6/esp32c3/api-reference/peripherals/gpio.html ; https://docs.espressif.com/projects/esp-idf/en/v5.5.6/esp32c3/api-guides/partition-tables.html ; https://sensirion.com/media/documents/33FD6951/662A593A/HT_DS_Datasheet_SHT4x.pdf

---

# Active room sensor development — 2026-11-25

`dev` is the sole active integration branch for the standalone ESP32-C3/SHT40 sensor. Fast-forwarded from 77ed5aa34e8b0321a6e48c98b8ef1aa1cb06b4e8 to existing public measurement commit 9b7281f7f384f7fe07b93e6e3f02d572165544d0; original experimental branch preserved. Independent repeat of `sh tests/run_host.sh` at that source: all seven Git blobs matched docs/MEASUREMENT_STAGE.md; GCC 14.2.0 and Clang 17.0.0, four executions exit 0, strict C11/UBSan. This is a development baseline, not an approved target firmware. SDK/dependency and full history audit, target build, physical I2C and transport freshness remain open. Heating safety work is paused; no boiler integration, OTA or changes to main/safety-control. Existing audit gates retained.

---

# Experimental measurement stage — 2026-11-25

Standalone measurement candidate: restart clearing, validated scheduling and freshness, SHT40 I2C adapter and ESP-IDF scaffold. Four actual host test executions exit 0 on GCC and Clang; target adapter not built or tested on hardware. See docs/MEASUREMENT_STAGE.md for scope, exact source hashes, commands and audit limitations. No transport or boiler control. No new CI.

---

# Independent partial audit — 2026-11-25

At fixed dev source 30b9ed7e266d55ca66ea0c270cee0dc3a34e5cdd, the original C11 test actually ran locally (GCC 14.2.0, strict warnings, nonrecovering UBSan, timeout 10 s, exit 0). Git blob SHA verified independently: src/sht40.c cc9dc7317de93b38ff49a5dba98af96d3c3c4385, src/sht40.h d40ace0c443e6d96e7a7eaa356e491494f330395, tests/test_sht40.c 5ff3c557559a7327c74967a903a84868483cbe5b. Command: gcc -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=undefined -fno-sanitize-recover=all -Isrc src/sht40.c tests/test_sht40.c -lm -o /tmp/sht_test; timeout 10 /tmp/sht_test. No ESP-IDF firmware, broker, provisioning, local OTA or physical validation in this tree. No new CI run; administrative permissions, secrets and environments not independently readable. Full history/dependency audit remains open; no new functional work or deployment.

---

# Current state — 2026-11-25

ESP32-C3 + SHT40 room sensor prototype: codec and temperature-only correction implemented; host C11 PASS reported at a908824. No target build, physical validation or complete firmware. Local MQTT integration with original SmartTherm is planned; exact topics, broker and freshness must be verified. GitHub-first development.

---
## Historical records (superseded where conflicting)

# Project state

2026-11-24: requirements baseline in dev. Experimental restart; no firmware, target build or hardware validation yet. Main remains unchanged. GitHub is the primary development location.

SHT40 codec and synthetic tests added. Temperature-only correction; humidity factory calibration only. Tests and target build not yet executed.

2026-11-24: host C11 tests PASS (exit 0) for a908824; all three source/test Git blob hashes matched the repository. Strict warnings enabled. ESP-IDF target build and physical sensor validation remain pending. No deployment or production integration performed.
