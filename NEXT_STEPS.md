# I2C recovery candidate — 2026-11-26

Review experimental source bd67087c9107a9056214fe1e6d008012318a2a64 before integration into dev. Complete artifact workflow and applicable build audit, then target-build this exact source and inspect retained evidence. Hardware tests: startup init errors, unplug/reconnect, stuck SDA/SCL, cleanup failure, repeated recovery, actual cadence, stale sample suppression, resource/heap and task-watchdog checks. Verify GPIO/pull-ups for board separately; no hardware deployment before approval. Existing audit/OTA/network gates and heating freeze remain. No extra paid/private CI.

---

# First successful ESP32-C3 target build — verified 2026-11-26

Baseline target compilation now confirmed; do not repeat solely for another green status. Finish installed dependency/advisory review and exact linked-component inventory. Prepare separately audited limited experimental artifact retention, with explicit source/SDK provenance and SHA256 manifest; verify ZIP and inner binaries after download. Then review recoverable I2C init, board pin constraints, persistent configuration and synchronized freshness; hardware testing remains mandatory before use. Keep MINIMAL_BUILD and OTA partition changes separate and reviewed. No flash, live heating, production deployment or private/paid CI. Existing source pin 08f487175b6995864285e420a72f0092a102c68f; workflow main f7d7fe764ed6d09a195ed349cd17a15b9180044a.

---

# ESP32-C3 CI input-copy fix — 2026-11-25

Inspect new run 45632347131 on workflow f7d7fe764ed6d09a195ed349cd17a15b9180044a, source still 08f487175b6995864285e420a72f0092a102c68f. Verify full logs and actual build result, generated files and checksums. First run reached verified SDK/compiler and Python inventory but not compilation. Keep full dependency/advisory and hardware gates open; no flash, OTA, paid CI or heating changes.

---

# Isolated ESP32-C3 CI — 2026-11-25

Inspect run 45630874212 / job 139263403475 and full logs, verify SDK commit, compiler, submodule list, installed dependency versions, actual build exit, generated partition table, sizes and output checksums. If failed, diagnose actual failure before reviewed fix; if passed, finish dependency/component advisory audit and add separately audited artifact retention before hardware testing. No flash/OTA, private CI or heating development. Source fixed at 08f487175b6995864285e420a72f0092a102c68f; workflow main 5db5202250e5ddb62639c6f290095633e4d21cb0.

---

# Build input review — 2026-11-25

Continue from unchanged functional sources at 5e405260e7c8d06625dcfb11cd01a1e3b23b5a9c. The live Python constraint text is accessible for review, not a verified local lockfile. Obtain a permitted audited target environment, verify actual SDK/toolchain bytes and resolved dependency hashes/advisories, inspect generated component and partition lists, then build the unchanged baseline. Keep MINIMAL_BUILD and OTA partition changes separate and reviewed. No repeated host regression solely for evidence; no flash, paid CI or heating development.

---

# SDK provenance extension — 2026-11-25

Continue from verified dev source 79b210eca3d01093bc34a08fb643dc234322427b. SDK first-level gitlinks and release archive metadata now available, but actual archive/toolchain hashes, submodule contents, resolved Python constraints, complete advisory and build component closure still require verification. Obtain a permitted audited build environment before target compilation; do not rerun existing host tests merely to repeat evidence. Keep heating frozen; no flash or paid CI.

---

# ESP-IDF build gate — 2026-11-25

Obtain a verified ESP-IDF v5.5.6 build environment (SDK commit 880a203ae603d08a94d06aa01e0d6761958e5e9e), toolchain archive checksum and resolved Python requirements/constraints, submodule/component closure and relevant advisories. Consider MINIMAL_BUILD as a separately reviewed change to reduce unused SDK dependencies; build the unchanged baseline first. If using public CI, first close administrative security gate. Run idf.py set-target esp32c3 and idf.py build, record exact versions, exit codes, map/size and artifact SHA256; no flash. Do not repeat host tests solely to duplicate prior evidence. Keep heating frozen.

---

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
