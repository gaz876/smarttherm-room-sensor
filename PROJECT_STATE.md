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
