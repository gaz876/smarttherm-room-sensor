# Isolated AP/STA reply-deadline regression — 2026-11-26

Isolated follow-up from protected PR #17 HEAD 0043c1e9498f22228db950144c0f6665ec83cf23. Pending Security 2 handshake reply now expires during session poll; borrowed buffer retained until sender completion/drain. New regression: old source fails assertion (exit 134); fixed source GCC 14.2.0 and Clang 17.0.0 strict C11 ASan/UBSan both exit 0. Test inputs match public baseline Git blobs; changed source d8b2758063b56c31c13afeb10c14b9ebf1e026c8 and test 0bf9f00b4391bbd54d50c1543dd8757ec3f80c4c. No live crypto/network/hardware, target build, workflow or startup activation. No private data, credentials or new dependencies. Existing branch remains frozen. Ruleset 28138548 active, no bypass. Cloud coordinator abandoned; use isolated branches and reviewed PRs. New branch has no workflow; default main workflows have only workflow_dispatch/main path-scoped push, permissions {}. Administrative secret/permission inventory unavailable via connector; no CI requested. Remaining gates: fresh DHCP status and pretrial event barrier, overflow cleanup, station handoff, identity, decrypted-buffer disposal, actual phone transport, dynamic memory and MQTT. Prior artifact 15088994612 archive SHA256 independently corrected to 0e6f102f495ccfc89b5b722433c0cbcce7e7b61796f4ed20b7c559bfb77a25fc.

---

# AP/STA task target build complete — 2026-11-26

Pinned source 647b0e4437a9f6125e313af0c315dbd56863e5b1, run 45760537263, artifact 15088994612 verified. Test checkpoint 729618d86862f8c050b2646899c3a720902c01a7. Next station-to-AP exclusive runtime handoff and queue allocation, trusted per-device identity, SDK decrypted-buffer disposal and reviewed real socket lifecycle, actual phone setup and DHCP/channel/cleanup tests. Keep activation blocked pending these gates. Protected NVS physical tests, dynamic heap/stack and MQTT broker remain queued. 153680 B free in factory partition; not runtime memory proof. No unrelated old tests/rebuild, automatic eFuse/OTA/heating or paid/private CI. PR #17/#20 draft; no main merge.

---

# AP/STA integration target build pending — 2026-11-26

Check run 45760537263/job 139709205908 for pinned source 647b0e4437a9f6125e313af0c315dbd56863e5b1, verify full log, linked symbols, artifact hashes/source manifest and footprint. Task integration currently uncalled; next actual exclusive station handoff and queue allocation, trusted identity/bootstrap, secure SDK credential buffer disposal, bounded live socket and physical AP/STA/DHCP/channel/cleanup. Provisional 5 s cleanup deadline is not preemption, event barriers require physical proof; lockout is not radio-off guarantee. No automatic eFuse/OTA/heating or paid/private CI. Queue retained: phone client, protected NVS hardware tests, dynamic RAM, MQTT compatibility and broker. No unrelated old tests or repeat build of unchanged source.

---

# Dual-netif AP/STA owner checkpoint — 2026-11-26

Continue from 6ba25177d679fb0fd5fa24596b173071d0810a23. Next bind exclusive network task, actual event barriers and bounded cleanup deadline, independent fresh DHCP event proof, bridge/transport lifecycle and trusted identity. Do not expose AP or credentials until applicable SDK buffer disposal and dependency gates reviewed. Target compile/build after relevant linked integration. Queue retained: initial phone setup, protected NVS hardware recovery/confidentiality, dynamic heap/stack measurement and MQTT compatibility/broker. One trial per window provisional; improve retry only after safe isolation. No old test repetition, automatic eFuse/OTA/heating, paid/private CI or main merge.

---

# AP/STA and runtime diagnostics checkpoint — 2026-11-26

Continue from 9a39e7bb984fd7e73694c2ad241169e525dcb583. New isolated APSTA configuration and memory probe tests complete. Next exclusive dual-netif owner and start/stop/drain integration, actual fresh DHCP and AP channel migration, trusted per-device identity and SDK decrypted-buffer disposal, approved bounded transport and phone client. Hardware gates: real protected NVS power-cycle/error recovery and flash confidentiality, runtime internal heap/stack low-water and largest block through handshake/trial/cleanup. No automatic eFuse/OTA/heating or plaintext fallback. MQTT preliminary author code confirms incoming numeric indoor/outdoor callbacks, but exact deployed topic/QoS/retain/staleness must be pinned and broker candidate audited before implementation. No unrelated tests or target build until relevant linked change. Prior validated artifact 15087259482; PR #20 draft not merged.

---

# Security 2 target footprint complete — 2026-11-26

Source db9d8f1666f0b0a40ee20eb7d3ae79abde9a4775, run 45755803953, verified artifact 15087259482. No repeat build of unchanged source. Next: secure identity/bootstrap and decrypted-buffer disposal review, bounded real transport and AP/STA/DHCP cleanup; real phone interoperability, runtime heap/stack peaks, connection and recovery. Account for 159952 bytes remaining in 1 MiB app before MQTT/web/OTA; do not change physical partition table or enable keys automatically. PR #17 and #20 remain draft; no main merge, hardware/OTA/heating or paid/private CI. Static footprint does not establish runtime readiness. Dependency and administrative gates remain.

---

# Security 2 footprint build in progress — 2026-11-26

Inspect run 45755803953/job 139694014961 for source db9d8f1666f0b0a40ee20eb7d3ae79abde9a4775, verify real compile/link, full logs, artifacts and exact source manifests; compare static flash/DRAM to prior build. CI PR #20 not for merge. No repeated old tests. Runtime crypto, heap/stack peaks, identity, SDK decrypted-buffer disposal, actual AP/STA/DHCP and Android client still not verified. Keep activation blocked, no automatic eFuse/OTA/heating or paid/private CI.

---

# Security 2 context checkpoint — 2026-11-26

Continue from 74156c8be6a72b99c8a76f0921aec780d6ea307d. Context ownership mock tests complete. Review applicable SDK dependency and decrypted-buffer disposal gates, trusted per-device salt/verifier provisioning and secure storage. Implement bounded single-owner transport and endpoint/bridge binding with real response completion, no plaintext fallback; actual AP/STA cleanup, DHCP and Android client. Audit isolated target build inputs, measure link footprint before activation and confirm real Security 2 interoperability. No old test repetition or automatic eFuse/flash/OTA/heating, no paid/private CI. Context is not wired into CMake or runtime and no real Security 2 handshake has been tested.

---

# Setup model integration checkpoint — 2026-11-26

Continue from 85cbc2c5b0953059281d700040fddc8c3bbcc741. Offline bridge tests complete. Next review exact SDK Security2 session and buffer ownership, authenticated per-device identity, AP/STA trial implementation and completion/cleanup semantics. Bind actual adapter and protected store only after applicable dependency and memory review; ensure input/output lifetime, bounded queues/callbacks, fresh DHCP and timeout behavior. No old test repetition; no automatic eFuse, plaintext fallback, hardware/OTA/heating or paid/private CI. Target build after relevant integration.

---

# Transport lifecycle checkpoint — 2026-11-26

Continue from c1a83001e86314be2f258fa3ea24df088e7142f2. New lifecycle host tests complete. Next integrate dispatcher, phone owner and lifecycle in an isolated adapter test, including timeout/stop propagation and actual clock after blocking operations; verify Security2 SDK session/buffer ownership and completion semantics, trusted per-device identity, Android client, AP/STA cleanup and DHCP, resource sizing and applicable advisories before exposure. Fixed 2 s budget provisional. No old test repetition, hardware, eFuse, OTA, heating or plaintext fallback. Target build only after relevant integration.

---

# Phone command dispatch checkpoint — 2026-11-26

Continue from dcb64b90902749e4fe5e58982e70aa0ee1bbd20c. Dispatch tests complete; no unrelated test repetition. Next bounded transport owner lifecycle including final response/timeout before AP teardown, queue/rate limits, monotonic deadline and real AP/STA trial cleanup, trusted link/DHCP observation. Review official SDK Security2 session/buffer lifetime, per-device identity, Android client and memory/partition/advisory gates before actual network integration. No hardware/eFuse/OTA/heating or plaintext fallback. No new target build until relevant integration.

---

# Phone request framing checkpoint — 2026-11-26

Parser and standalone host test verified at 8f742d170bb8664dacc96013f60da39c20e0bef7; no need to rerun unchanged tests. Next: design bounded reply/dispatch and transport lifecycle with explicit authenticated session ownership, rate limits, cleanup and close/response ordering. Verify SDK Security2 buffer ownership, per-device identity enrollment, Android client, actual AP/STA trial and fresh DHCP, partition/heap budget and applicable advisories before network integration. Parser does not authenticate or validate profile semantics. No plaintext fallback, automatic key/eFuse writes, hardware, OTA, heating or paid/private CI. Target build only after relevant integration.

---

# Phone setup transport checkpoint — 2026-11-26

Target run 45740340743 independently verified; do not rebuild unchanged source. New phone session owner tests passed; review protected transport input/buffer lifecycle, Mbed TLS advisory applicability, per-device identity, Android client protocol and actual AP/STA ownership. Confirm flash/heap budget before adding transport. No plaintext fallback, automatic eFuse writes, hardware or unrelated test repetition.

---

# Target integration build pending — 2026-11-26

Inspect run 45740340743/job 139643493540, all steps and full logs, exact source blobs, linked components, new partition map and independently downloaded artifact. Fix actual failure if necessary. Do not repeat old tests or flash. Continue protected phone enrollment and write integration only after applicable review; no automatic eFuse writes or plaintext fallback.

---

# Secure NVS startup integration checkpoint — 2026-11-26

Build exact experimental integration for ESP32-C3 after applicable isolated CI audit. Verify binary size, linked dependency closure and partition offsets; no flash. Continue authenticated phone setup, trusted per-device identity and write/recovery integration. Default key -1 disables Wi-Fi; do not choose or generate hardware key without explicit approval and verified device state. No unrelated test repetition.

---

# Secure store candidate checkpoint — 2026-11-26

New isolated store mock tests pass. Review SDK partition ownership and boot recovery; prove actual protected per-device key and separate partition layout, choose trusted first enrollment method, implement authenticated phone transport and integrate dedicated network task. Keep actual activation blocked without prerequisites. Target compile and linked dependency audit still pending. No eFuse generation, plaintext fallback, erase, deployment, or unrelated tests.

---

# Wi-Fi enrollment integration — 2026-11-26

Station adapter target compile and artifact verification complete; no need to repeat this build solely for evidence. New standalone enrollment transaction host tests pass. Continue with reviewed secure storage backend and trusted per-device setup identity, no plaintext fallback or automatic eFuse writes; implement authenticated phone setup and real AP/STA trial, fresh DHCP proof and explicit persistence, reboot recovery and session zeroization. Bind BOOT and station adapter into dedicated network task without changing measurement logic. Verify real linked Wi-Fi dependency closure and target build only after functional integration. PR #17 remains draft; no flash/OTA/heating. Prove physical board storage state and actual network behavior before use. No unrelated test repetition.

---

# Wi-Fi target and enrollment gates — 2026-11-26

Inspect run 45735143422 when terminal, verify full log, compiled adapter object, artifact and source blobs. Source 6a986225ed3bf2312ac7adf1524181e72f6c9fc0; later journal-only changes allowed. Do not claim runtime networking from unused adapter. Continue protected enrollment/storage design docs/WIFI_ENROLLMENT.md, implement authorized profile lifecycle and separate task; validate actual GPIO9, DHCP, event barriers, resource usage and loss recovery. Stock wifi_prov_mgr switches to FLASH; don't use as-is. No automatic eFuse writes or plaintext fallback. No repeated unrelated tests.

---

# Wi-Fi binding target build next — 2026-11-26

Audit direct SDK component closure and compile exact candidate on ESP32-C3, verify artifacts. Adapter is uncalled so target link/size does not prove working network task. Then authenticated provisioning/enrollment and secure persistent profile interface, separate task integration, on-device tests for delayed vendor events, DHCP, GPIO9, resource use and driver recovery. No real credentials or AP until protected provisioning is ready. Do not enable irreversible eFuse features without approval. No repeat unrelated tests.

---

# Wi-Fi ESP-IDF binding next — 2026-11-26

Bind tested owner to actual ESP-IDF transport with verifiable stop/drain and fresh DHCP lease; callback producer generation must be bound before posting, not guessed from current attempt. Validate GPIO9 and use 50 ms debounce/3 s hold after release. Protect credential storage before provisioning; do not enable AP or persist passwords yet. Target and bench tests pending.

---

# Wi-Fi adapter next — 2026-11-26

Implement and test actual ESP-IDF event/task adapter, protected setup and credential storage design, BOOT and DHCP, stop/drain, generation and action-failure handling. Do not expose AP or persist real passwords until protected setup and storage verified. Target build and actual device testing pending. No repeat unrelated tests.

---

# Runtime NVS integrated; Wi-Fi next — 2026-11-26

NVS source 6bd64d225c4c97884760ed0e25c4048c626b521b built and integrated in dev 759921b744300f304830292ca35060011c867de6. Next: autonomous Wi-Fi policy and tested ESP-IDF adapter, protected first boot/manual BOOT setup, DHCP default, indefinite nonblocking reconnect without automatic AP on disconnection, credential storage and provisioning review. No real credentials in sources or CI. No automatic NVS erase or irreversible eFuse changes; secure storage design must precede real credential use. Physical NVS/power-loss and concurrent tests outstanding. No hardware flash, OTA, heating, paid/private CI.

---

# Runtime settings command candidate — 2026-11-26

NVS startup/storage baseline is integrated in dev 9e679f776ed56a97c975b9e1733d9aefe63e9ab8. Target-build the new runtime command candidate with retained artifacts, verify exact source blobs and archive; integrate experimental PR into dev after review. Future protected Wi-Fi/web path must authenticate, serialize command clients, consume result before submitting another, rate-limit explicit saves and reject invalid/stale measurement or unsafe settings status. Credential storage review remains before Wi-Fi. Physical NVS persistence/power-loss and FreeRTOS concurrency not proven. No hardware flash, OTA, heating or paid/private CI.

---

# Experimental NVS settings — 2026-11-26

Target build and archived evidence complete for be95be3d3676c24a077a443bdf4c01c9f7309980; integrate reviewed PR #12 into dev if all later changes are tests/docs only, verify build-input blobs unchanged. Next: protected Wi-Fi provisioning and persistent network settings with credential handling review (current measurement NVS unencrypted; no credentials yet), nonblocking reconnect and manual BOOT setup; implement single-owner runtime measurement configuration command path and explicit save/reset/readback before applying new settings; connect synchronized snapshot to authorized future consumers and reject invalid/stale or unsafe configuration fallback. Bench verify real NVS power-loss/restart, fault handling, actual I2C recovery and concurrent access; no live heating/flash/OTA without required clearance. Prior dependency/advisory audit before stable release. Continue only new relevant tests. No private/paid CI.

---

# ESP32-C3 I2C recovery target build verified — 2026-11-26

Target compilation and artifact verification complete for 598f896a1d3e0ca3d66b0b2674f9eee31528214e. Integrate reviewed PR #11 into dev; verify resulting source blobs unchanged. Next: versioned validated NVS measurement settings with persistent correction/interval, explicit reset, synchronized freshness, then Wi-Fi provisioning and MQTT protocol compatibility. Retain previous tests; run new relevant tests only. Complete full applicable dependency/SBOM review before stable release and bench verify GPIO/pull-ups and physical I2C recovery before use. No flash, heating control, OTA or paid CI.

---

# Public artifact CI enabled — 2026-11-26

Inspect run 45727766399 / job 139601840682 when terminal. Check actual compilation, every step, logs, source blobs, linked components, partition table, artifact digest and ZIP contents/inner SHA256. PR #11 remains draft until target evidence reviewed. Complete applicable advisory review before stable release. Next functional phase: versioned validated NVS measurement settings, single-record commit, no writes on each measurement, preserve existing settings on recovery, synchronized fresh snapshot. No blanket NVS erase on init failure; keep factory reset explicit. Existing host I2C/GPIO tests need not be repeated without relevant code changes. No hardware flash/OTA or heating work.

---

# Independent GPIO and artifact preparation retest — 2026-11-26

Host regression at fixed GPIO source 598f896a1d3e0ca3d66b0b2674f9eee31528214e now independently complete; do not repeat solely for evidence. PRs #10/#11 remain experimental/draft. Remaining: applicable upload/SDK dependency audit and administrative review, actual target build, archive and inner-file verification, separate reviewed SBOM generation, bench confirmation of board/pull-ups and I2C recovery. No flash, production deployment, OTA, heating changes or private/paid CI.

---

# Provisional SuperMini GPIO profile and artifact CI preparation — 2026-11-26

Review PRs #10/#11 at pinned source 598f896a1d3e0ca3d66b0b2674f9eee31528214e and workflow 416ad8810712e2c1c10a2175d6443c617cc217a6. Complete applicable upload action and SDK dependency review and administrative gate before enabling workflow. Then actual target compile, archive download and independent checksum/content verification; separate reviewed SBOM generation. Full host regression rerun on exact source, bench confirm board revision, 3.3 V pull-ups and physical I2C recovery. No flash, production/OTA, heating or paid/private CI. Revert experimental changes by reviewed commit, not history rewrite.

---

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
