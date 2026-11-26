# 2026-11-26 — Dedicated APSTA static queue candidate

- APSTA task context now embeds one static FreeRTOS queue with eight room_apsta_event records; the owner no longer accepts an arbitrary caller-supplied queue handle. No per-window allocation/free. Bind is single-use, rejects missing task or queue creation failure before callback registration, and retains persistent context on ambiguous registration failure. Context must be zero-initialized once and never moved/freed before reboot. Eight records is the existing mock capacity, not a measured hardware sizing guarantee.
- Added regression checks for the actual queue length/item size/backing storage, allocation failure before registration and duplicate bind rejection. New candidate is not yet tested or built. Previous source 89087c8062e0b0bb8d1a4111b7e6573eb50ef5a1 passed target build and all five GCC ASan/UBSan tests in CI 45777612825, artifact 15097126016 (CI digest 9f2d0ce62c8ddb20e2b3466bf1649cbe00c1a7662b43deef78233cdd11cfc367). Clang unavailable in CI; independent archive verification pending.
- Still required: unified actual STA/APSTA owner, bounded cleanup, sender/identity/credential lifecycle, real DHCP freshness and physical heap/stack/queue measurements. No startup activation, protected branch merge, eFuse changes, new dependencies or deployment.

---

# 2026-11-26 — APSTA capture isolation candidate

- Persistent event callbacks now ignore normal STA traffic outside the APSTA setup window. Capture starts only in the exclusive owner's open path and ends on successful full teardown (or rejected open before a window exists). Overflow remains sticky; the change never clears fault state. Caller must still prove station cleanup before opening APSTA. This gate does not establish freshness of events produced late by the SDK.
- Added regression assertions for inactive callback traffic before first open, between windows, and after poisoned teardown. New candidate not yet compiled or executed. The separate CI run 45776861306 is pinned to the preceding BOOT fix 819ec2594c153860ec3b84a31749c7236c4eed87; do not attribute its result to this candidate.
- Remaining: actual single-owner runtime handoff, shared cleanup contract, dedicated queue allocation, physical DHCP/event validation, identity and credential/phone transport gates. No runtime activation, no merge to protected PR #17, no dependency changes, no deployment or paid/private CI.

---

# 2026-11-26 — Deferred physical BOOT request during station cleanup

- PR #23 isolated candidate: preserve authorized manual setup request while old transport stops asynchronously. No new start before successful stop; no repeat BOOT extends original 300 s window. Recheck setup readiness and deadline while waiting; cancel on readiness loss or expiry. Manual gesture has priority over ordinary STA retry in that step, and previous generation events cannot activate the new owner. No real AP activation or new credentials.
- Exact owner header dc1074cbfc872cff9e9f0d2f36065ff0171c6720, new handoff test 16ed38314bd516a28c6b426e1884618558c5b71e, unchanged original owner test 8db32f6debfde00b2a858f4c5b07ce4b837d7f9c and policy c54bd7ee5916dbc050e4199dce58bb10d2000e06. Actual four local strict C11 ASan/UBSan runs passed (GCC 14.2.0 and Clang 17.0.0, new and existing owner tests).
- Previous APSTA marker source 5cffd1dbb4592bd1bcb480c035bdc6e217135ff1 target build and GCC session/events/task integration tests passed in run 45775299132; Clang unavailable in CI image. Artifact 15095562424, CI digest c0b62194895adfb6a57849f449679ea86da2212851f15ec3a10d6f7904d15969; independent archive verification pending. New owner candidate target build pending.
- Next: validate actual exclusive runtime handoff and bounded cleanup, dedicated queue allocation, real event/DHCP freshness, identity and credential disposal gates. Station-only runtime still passes setup_ready=false; no physical tests, activation, protected-branch merge, dependency changes, paid CI or Cloudflare changes.

---

# 2026-11-26 — Pretrial marker and poisoned queue cleanup candidate

- Isolated PR #23 only, based on a58aee12e63bbf52e70ba84b484a0137ebed4f54. Do not merge into protected PR #17 baseline.
- Trial cannot start in the task adapter until a distinct default event-loop marker has been acknowledged and the application event queue drained. An aborted pretrial marker is retired, never reused as a teardown marker.
- Teardown uses bounded event discard after its own markers, independently of the sticky overflow fault. The fault continues to block new setup windows until controlled reboot. No overflow reset or automatic recovery is introduced.
- Focused local adapter harness with the exact changed task and event C files, simplified SDK/session mocks: GCC and Clang strict C11 with ASan/UBSan both exit 0. Covered post failure/retry, delayed acknowledgement, pretrial stale event, overflow during operation/teardown and pending marker, independent teardown marker, and blocked reopen. Existing repository regression tests extended but not executed in this session. No target build or physical tests; default event-loop barrier is not independent proof of a new DHCP lease.
- Next: run repository integration tests, audit and build exact candidate for ESP32-C3; verify actual event producer timing, lease freshness and physical AP/STA cleanup. Exclusive station handoff, queue allocation, identity, credential lifetime, phone transport, MQTT and hardware gates remain. No runtime activation, protected-branch merge, workflow/dependency changes, deployment, Cloudflare changes or paid CI.

---

# Isolated AP/STA follow-up — 2026-11-26

Protected baseline 0043c1e9498f22228db950144c0f6665ec83cf23 (PR #17); separate experiment, no changes to protected branch. Require running DHCP client in addition to matching got-IP, live STA link and current IP. This is not proof of DHCP lease freshness; pretrial SDK event-loop barrier and physical testing still required. Stop pending Security 2 replies at the existing 2 s deadline without releasing sender-owned buffer before drain. Six actual host mock runs (session, events, task × GCC 14.2.0 / Clang 17.0.0, strict C11, ASan/UBSan) exit 0. No startup, live transport, new dependencies, workflow changes, hardware, deployment or target build. Previous target archive 15088994612 corrected SHA256: 0e6f102f495ccfc89b5b722433c0cbcce7e7b61796f4ed20b7c559bfb77a25fc (local verified). Next pretrial event barrier, overflow teardown, exclusive station handoff and queue allocation; identity/credential lifetime gates remain. No activation or merge.

---

# AP/STA task target build verified — 2026-11-26

Run 45760537263 all four steps success, verified artifact 15088994612, CRC, 19+5 hashes, all 41 input paths. App 894896 B (+6272), factory free 153680 B, static DRAM 116622 (+8). Same partition/image/dependency manifests and linked archive set. Uncalled integration APIs retained, no real network, dynamic RAM or hardware proof. PR #20 remains draft.

---

# AP/STA task and DHCP event binding — 2026-11-26

Added experimental session coordinator, queued SDK event capture and exclusive task boundary; six actual GCC/Clang ASan/UBSan host test runs passed, with test-only cleanup for quarantine path. CMake retains uncalled integration APIs. Target source 647b0e4437a9f6125e313af0c315dbd56863e5b1; test checkpoint 729618d86862f8c050b2646899c3a720902c01a7. CI PR #20 run 45760537263 in progress. No live socket, real startup activation, hardware or deployed change.

---

# Dual-netif AP/STA owner candidate — 2026-11-26

Added isolated direct SDK APSTA owner and mock test at 6ba25177d679fb0fd5fa24596b173071d0810a23; actual GCC/Clang ASan/UBSan runs exit 0, all five published blobs match. Start, explicit single trial, stop and two-phase drain tested. Real DHCP, barriers, transport, CMake/startup and hardware not wired; one trial per window provisional. No new target build.

---

# AP/STA preparation and memory instrumentation — 2026-11-26

Added two isolated headers and six test/mock files at 9a39e7bb984fd7e73694c2ad241169e525dcb583. Four actual GCC/Clang ASan/UBSan host runs passed, all eight published blobs match. Reviewed SDK AP/STA shared handlers, NVS read-key path, and initial SmartTherm MQTT numeric callbacks. No actual radio, phone interop, hardware encryption or RAM measurement, broker implementation, new target build or workflow change.

---

# Security 2 target footprint build verified — 2026-11-26

Run 45755803953 all four steps success, artifact 15087259482 independently checked: CRC, 19+5 hashes and 32 input entries; exact SDK image and dependency manifests unchanged. App 888624 bytes (+41200); 159952 bytes free in factory; static DRAM 116614 (+16). ELF retains Security 2 and four adapter symbols. No real network or dynamic heap measurement. PR #20 not merged. Prior pending checkpoint superseded.

---

# Offline Security 2 packet binding — 2026-11-26

Added direct SDK adapter and mock integration test; both actual GCC/Clang ASan/UBSan runs pass, five published hashes match. Experimental target source db9d8f1666f0b0a40ee20eb7d3ae79abde9a4775 includes uncalled link retention and Security 2 only. Dedicated PR #20 run 45755803953 started, result pending. No real transport/AP, credentials or deployment.

---

# Offline Security 2 SDK context candidate — 2026-11-26

Reviewed pinned SDK APIs and transport ownership; added unlinked context wrapper and mock SDK tests at 74156c8be6a72b99c8a76f0921aec780d6ea307d. Final GCC/Clang ASan/UBSan mock tests exit 0, four published hashes match. Initial missing standard header fixed. No real crypto, transport, target build, CMake/workflow or deployment change.

---

# Setup model integration — 2026-11-26

Added offline integration bridge and host test at 85cbc2c5b0953059281d700040fddc8c3bbcc741. Both GCC/Clang ASan/UBSan actual runs exit 0; published hashes match. Slow live check blocks write, late completed write read back without late response, explicit cleanup gates. No actual SDK/AP/STA or hardware integration, new CI or target build.

---

# Setup transport lifecycle model — 2026-11-26

Added isolated model and test at c1a83001e86314be2f258fa3ea24df088e7142f2. GCC/Clang strict ASan/UBSan actual runs passed, both published blobs match local tests. One pending request, final reply deadline and explicit cleanup gate. Not connected to SDK or dispatch. No new CI requested or target build.

---

# Experimental phone command dispatch — 2026-11-26

Added isolated command dispatch and fixed reply at dcb64b90902749e4fe5e58982e70aa0ee1bbd20c. Two actual strict ASan/UBSan host runs passed on GCC/Clang, all five source/test blobs match pinned GitHub. No actual network, flash, target build, new dependencies or CI requested.

---

# Phone request framing candidate — 2026-11-26

Added structural request parser and isolated host test at 8f742d170bb8664dacc96013f60da39c20e0bef7. GCC and Clang ASan/UBSan runs passed, 65536 length combinations each; both published blobs match. No transport, credentials, firmware build or deployment. Existing archive hashes rechecked locally. PR #17 remains draft.

---

# Secure NVS target evidence and phone setup owner — 2026-11-26

Verified existing target build, all archive hashes and source inputs. Added and host-tested isolated phone setup owner. No live transport, phone client, eFuse or new CI. Dependency review partial; network activation remains gated.

---

# Secure NVS integration CI — 2026-11-26

PR #19 merged isolated workflow pin only; target run 45740340743 in progress at checkpoint. No new firmware result yet. Verified old partition offsets and two compile-guard rejection checks.

---

# Experimental NVS startup integration — 2026-11-26

Added dedicated network task startup, explicit disabled-by-default key selector, separate experimental partition and protected profile load into station adapter. 16/16 new mock startup cases passed. No target/hardware confirmation, phone setup or actual writes.

---

# Experimental secure Wi-Fi store — 2026-11-26

Added isolated explicit HMAC-backed NVS adapter, 16 actual mock SDK executions passed. No activation, partition changes, hardware or target build. Prior tests not repeated.

---

# Wi-Fi station CI verification and enrollment transaction — 2026-11-26

Verified existing target build and archived hashes independently. Added and tested a standalone enrollment transaction with fail-closed storage gate and runtime uncertainty latch. No live networking, encrypted NVS backend, UI, deployment, or new CI. Prior tests not repeated.

---

# Wi-Fi target CI and provisioning review — 2026-11-26

PR #18 workflow-only merged; run 45735143422 building, not yet verified. Reviewed stock provisioning manager and HMAC provider; documented credential storage/logging and irreversible generation risks. No hardware or network activation.

---

# Experimental station binding — 2026-11-26

Added ESP-IDF station-only adapter candidate and mock tests; six final executions passed. Real target and network task integration pending, no credentials or AP. Existing main.c unchanged.

---

# Wi-Fi transport owner — 2026-11-26

Added and tested generation-aware transport lifecycle and BOOT gesture. Two actual host runs passed. No firmware or hardware changes.

---

# Wi-Fi policy experiment — 2026-11-26

Standalone policy and two passed host runs; no target networking or firmware integration. See docs/WIFI_POLICY.md.

---

# Runtime NVS build and integration — 2026-11-26

PR #14 merged to dev 759921b744300f304830292ca35060011c867de6 after actual target run 45730901218 and independent artifact 15077320314 verification (20 files, checksums, all 14 inputs). No source differences after merge. App 220256 bytes, static DRAM 62458. No repeat host tests or hardware use.

---

# Experimental runtime settings command — 2026-11-26

Added single-owner runtime NVS save/reset/readback and ticket result, responsive I2C backoff, exact active config snapshot. Eight actual host test executions passed on GCC/Clang; no duplicate standalone I2C/GPIO tests. Target build pending; no new external dependencies, network, heating or deployment.

---

# Experimental NVS settings — 2026-11-26

Implemented versioned measurement settings and synchronized snapshot; six final new host test executions passed (settings, mock NVS adapter and actual app with mock SDK on GCC/Clang), no repeat of standalone I2C/GPIO tests. Target source be95be3d3676c24a077a443bdf4c01c9f7309980; CI PR #13 merged, run 45729696843 success; downloaded artifact 15076615709 verified ZIP and inner manifests. App 219328 bytes, static DRAM 62426 bytes. User-facing settings save/reset, physical persistence and real task concurrency not yet verified; no production changes.

---

# ESP32-C3 I2C recovery target build verified — 2026-11-26

Run 45727766399 SUCCESS; downloaded artifact 15074763831 independently verified (20 files, ZIP and inner checksums). ESP32-C3 image and GPIO profile confirmed; no hardware or production changes. No repeated I2C/GPIO host tests.

---

# Public artifact CI enabled — 2026-11-26

Reviewed and merged standalone workflow PR #10; main f0e9604476b9f26785fda453da1619a1399418a8, run 45727766399 / job 139601840682 started. New firmware and downloadable artifacts not yet verified. Dependency advisory findings recorded; no host-test repetition or production changes.

---

# Independent GPIO and artifact preparation retest — 2026-11-26

Eight actual host test executions, 20 compile-time cases, exact workflow syntax and six synthetic export cases passed on fixed GPIO source 598f896a1d3e0ca3d66b0b2674f9eee31528214e / workflow 416ad8810712e2c1c10a2175d6443c617cc217a6. No new functional change, CI or hardware use.

---

# Provisional SuperMini GPIO profile and artifact CI preparation — 2026-11-26

GPIO mapping restricted to SDA4/SCL5; two actual host GPIO runs and 14 static-assert compilation cases passed. Artifact candidate now pins new source and verifies copied binary hashes; five actual synthetic export tests passed. Both PRs remain draft. No new CI or target/board verification.

---

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
