# Measurement regression checkpoint — 2026-11-25

State-machine host retest complete at e433dc0330ca80b6cd97ec432cb8b2db6375404a. Existing SDK/toolchain and CI audit blocker remains. Continue applicable audit and target build; no heating development. Do not repeat this test merely to duplicate evidence.

---

# Active sensor audit checkpoint — 2026-11-25

1. Obtain an audited ESP-IDF v5.5.6 target environment without paid resources; verify actual toolchain and dependency checksums, relevant advisories and component set, then run idf.py set-target esp32c3 and idf.py build. SDK download blocked by sandbox network; public CI requires its existing security gate. Do not use unverified binaries or claim target success.
2. Before hardware use restrict GPIO selection to verified board pins, preserve BOOT and USB where used, verify pull-ups, add recoverable bus/device initialization and tests. Establish explicit 4 MB partition budget for NVS and dual-slot OTA, verify actual image fit and rollback later. These are static findings/planning only, not implemented changes.
3. Continue existing dev sequence after applicable audit gate: persistent validated measurement settings, synchronized snapshot, protected Wi-Fi, MQTT protocol, UI, local OTA and hardware tests. Keep safety-control frozen and main unchanged.

---

# Active priority — standalone room sensor — 2026-11-25

1. Continue from dev measurement baseline 9b7281f7f384f7fe07b93e6e3f02d572165544d0. Close applicable SDK/dependency/history audit and build for ESP32-C3 without paid runners. Preserve exact test evidence.
2. Verify hardware I2C, flash size and pinout when available. Add persistent settings and synchronized fresh snapshots, then protected local Wi-Fi and MQTT broker/client and versioned sensor interface. Retain existing REQUIREMENTS.md, including temperature-only correction and no cloud dependency. Finalize the protocol before integration; no SmartTherm firmware edits.
3. Web UI, local OTA and independent integration/target/hardware tests. Temporary experimental branches must return through reviewed changes to dev; keep history. Freeze safety-control and related heating work; main remains unchanged. All previous security and hardware gates remain in force.

---

# Experimental measurement checkpoint — 2026-11-25

Complete actual SDK/dependency audit and ESP32-C3 target build; verify exact board pinout, sensor module, flash and real I2C failure/recovery on hardware before integration. Add persistent settings and synchronized snapshot/transport freshness, then audited local Wi-Fi/MQTT integration. Existing audit gates for CI, heating and production remain in force. No heating actuation or deployment. See docs/MEASUREMENT_STAGE.md.

---

# Independent audit gate — 2026-11-25

Complete remaining history/dependency/CI audit; confirm datasheet boundaries and invalid/stale handling, including restart semantics; only after audit closure proceed to ESP-IDF target and isolated local MQTT integration. Preserve temperature-only correction, 10 s default, offline operation and no heating actuation. No new functional work during audit.

---

# Current next steps — 2026-11-25

Verify ESP-IDF, C3 pinout and SHT40 datasheet; audit autonomous public CI; validate codec/freshness, target build, Wi-Fi provisioning, MQTT broker/client authorization and exact SmartTherm topics, web UI and local OTA design. Hardware and deployment pending. No private data or paid runners. GitHub-first; synchronize public journals to private after each checkpoint.

---
## Historical records (superseded where conflicting)

# Next steps

1. Verify ESP-IDF version, board pinout and SHT40 datasheet.
2. Audit pinned CI dependencies and create standalone build workflow with read-only permissions.
3. Implement and test sensor decoding and freshness policy.
4. Implement Wi-Fi provisioning and persistence, MQTT authorization, web UI and local OTA.
5. Verify target build, hardware and safe stale-data handling before deployment.
