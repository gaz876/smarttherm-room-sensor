# I2C recovery candidate — 2026-11-26

At bd67087c9107a9056214fe1e6d008012318a2a64 added reusable retry state, ESP-IDF adapter changes and host regression. Six real host test runs passed on GCC/Clang; ten local blobs match. Experimental only; no target build, hardware or production use.

---

# First successful ESP32-C3 target build — verified 2026-11-26

Run 45632347131 / job 139268608712 SUCCESS. Verified actual target compilation, 1003/1003 Ninja steps, generated BIN/ELF/MAP, partition table, output sizes and runner checksums in full log. Nine input blobs match dev; no downloadable artifacts retained. Documentation only.

---

# ESP32-C3 CI input-copy fix — 2026-11-25

Inspected failed run 45630874212, reproduced ownership error locally and verified replacement copy command. Merged one-line CI fix PR #9 without reducing isolation. New run 45632347131 queued; target success pending.

---

# Isolated ESP32-C3 CI — 2026-11-25

Created and merged standalone pinned-image target build workflow via public PR #8. Actual first Actions run 45630874212 started, result pending. No firmware code change, artifact publication, paid runner or deployment.

---

# Build input review — 2026-11-25

Read official live Python constraints via web, recorded exact recommended compiler archive metadata, confirmed default full-component build and default single-app partition choice. Rechecked unavailable local toolchain and failed alternate download. Documentation only; no build or deployment.

---

# SDK provenance extension — 2026-11-25

Enumerated 23 pinned top-level SDK gitlinks, recorded official SDK archive size/digest metadata, confirmed two upstream advisory fixes in v5.5.6 and rechecked local build blocker and public workflow triggers. No actual SDK installation or target build; documentation only.

---

# ESP-IDF build gate recheck — 2026-11-25

Rechecked current dev, direct SDK component manifests and Python requirements; identified need to audit resolved constraints and actual component closure, and optional MINIMAL_BUILD. Reconfirmed unavailable target environment and inaccessible CI administrative checks. Documentation only; no target build, CI, code or configuration changes.

---

# Measurement regression checkpoint — 2026-11-25

Reproduced existing state-machine test on GCC and Clang, 2/2 exit 0, five matching Git blobs. Details in PROJECT_STATE.md. Documentation only.

---

# SDK audit checkpoint — 2026-11-25

Verified official v5.5.6 tag and recommended target compiler manifest; independently reran original codec on two host compilers, 2/2 exit 0 with three matching blobs. Documented SDK/network blocker, GPIO constraints, initialization recovery and partition planning. Documentation only; no target build, source/configuration change, CI or deployment.

---

# Development consolidation — 2026-11-25

Fast-forwarded dev from 77ed5aa34e8b0321a6e48c98b8ef1aa1cb06b4e8 to existing public 9b7281f7f384f7fe07b93e6e3f02d572165544d0 without rewriting history or changing executable files. Independently verified all seven host-test blobs and repeated four host executions (GCC 14.2.0, Clang 17.0.0, exit 0). Preserve historical branches. Heating development paused; no main merge, CI request, deployment or firmware release. Rollback if needed by reviewed revert, not force-push.

---

# Experimental measurement stage — 2026-11-25

Added independent room_measure state machine and synthetic tests over the unchanged SHT40 codec, ESP-IDF C3/4 MB scaffold, separate I2C command/read with checked waits, build-time interval and runtime configuration API. Host regression 4/4 actual executions. No target build, deployment, CI, MQTT or production configuration changes.

---

# Independent partial audit — 2026-11-25

Repeated original codec test against three verified blobs, exit 0. Documentation only; no code, configuration, workflow, or release changed. This does not certify target firmware or MQTT.

---

# Current checkpoint — 2026-11-25

Codec host test result recorded at prior dev HEAD 5f5673f24183033d06b4d902c417e8062d9f9ad9. No new executable changes or target build in this documentation cleanup.

---
## Historical records (superseded where conflicting)

# Changelog

## 2026-11-24
- Established standalone requirements and development journals.
- No production integration or firmware release.

- Added SHT40 codec and synthetic tests; removed humidity correction at user request.
