# 2026-11-27 — C3 ISA qualification and timed channel close, experimental

- Exact tested source 35099c9dc1caad6c4931ffacc56e6bcc3f026f0c, parent 79f9e0f3e373a9456a58ac806cf3c79ad4e980d0 (draft PR #45). Nine changed/added files, all nine Git blob hashes matched independently generated and tested local inputs before branch publication. No CMake/startup/workflow/dependency/configuration change; ESP admission branch unbound, timed channel API optional and uncalled by actual Wi-Fi owner. Previous source/branches preserved. Source-only audit: no secrets or private inputs, no new listener/credentials/network calls; only local lifetime and deadline contract. Full repository administrative/dependency audit remains open.
- Reproduced portability failure in original admission header with Clang 17 --target=riscv32-unknown-elf -march=rv32imc -mabi=ilp32 -ffreestanding -std=c11: ATOMIC_INT_LOCK_FREE=1, static assertion requires 2, exit 1. This is a generic RV32IMC compiler probe, not a test of the vendor compiler. ESP-IDF v6.1.1 FreeRTOS documentation (https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-reference/system/freertos_idf.html, Critical Sections and Single-Core Mode) describes taskENTER_CRITICAL/taskEXIT_CRITICAL with portMUX, and single-core C3 behavior. New ESP_PLATFORM branch uses short task-context-only critical sections for count/close; no blocking calls inside section, no ISR use, same 32-lease experimental bound. Original lock-free host variant retained. Never copy leases, reuse the gate, or free context until callback source stopped.
- New compile-only RV32IMC probe with declaration-only FreeRTOS shims: clang --target=riscv32-unknown-elf -march=rv32imc -mabi=ilp32 -ffreestanding -std=c11 -Wall -Wextra -Werror -pedantic -DESP_PLATFORM -Itests/mock_rv32_admission -Isrc -c tests/test_room_admission_rv32.c -o probe.o. Exit 0, ELF32 RISC-V/RVC. nm -u shows only the three mock portMUX_INITIALIZE/taskENTER_CRITICAL/taskEXIT_CRITICAL declarations. No libatomic dependency in this object; not linked, not actual FreeRTOS or vendor SDK qualification.
- Added room_phone_channel_close_until, local monotonic clock and explicit backend window/deadline callback. Fixed deadline cannot be extended; check time before/after backend operation and completion, sticky quarantine on expired/reversed/invalid clock, stale window or changed contract. Timed/legacy attempts cannot be mixed. Preserve outstanding reply on timeout before completion; if completion already happened, no transition is granted after expiry. Legacy APIs retained for unchanged callers. Actual owner/session propagation and real callback backend remain unimplemented.
- Local GCC 14.2 and Clang 17: final set 18 executed programs exit 0, 16446 assertions total. For each compiler, two admission modes (default host atomic and ROOM_ADMISSION_CS_TEST), 1361 assertions each; for each mode, real channel and real SDK worker linked to host mocks, legacy 1052 and ROOM_ADMISSION_TIMED_TEST 1152 each; channel deadline unit/integration 232; original drain model/channel 581 and channel/SDK worker 280. Strict C11 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -pthread -no-pie; timeout 15 s each. Include paths -Itests/browser_worker/mock_channel -Itests/mock_sdk_stop -Isrc. First compile tests/test_room_admission.c; second link main/room_phone_channel.c main/room_sdk_stop.c tests/test_room_admission_sdk.c with combinations of the two mode defines; third link main/room_phone_channel.c tests/test_room_channel_deadline.c; original drain tests link same channel and optional worker as prior entry. Host mocks are not timing or race-freedom proof. A local test-edit syntax error was corrected before final runs; a whitespace mismatch in an unpublished detached candidate was corrected and all nine hashes rechecked.
- Blockers: local idf.py, riscv32-esp-elf-gcc, qemu-system-riscv32 and Docker absent; direct public archive curl failed DNS (6), web archive inaccessible, previous administrative reads GitHub branch protection 403 and actions/permissions 400 remain unresolved. No new CI requested. No ESP-IDF build/QEMU/hardware evidence for this source. Existing tested combined candidate remains separate. No protected merge, OTA, real network activation, heating or config changes. Rollback by retaining previous draft/abandoning new candidate, no history rewrite.
- Next: propagate exact outer cleanup deadline into actual room_wifi_idf/room_wifi_apsta_link/room_apsta_task/room_apsta_session interfaces with complete source regression fixtures, and use timed channel API; qualify real task-context gate lifecycle and bounded callback/handle ownership in pinned ESP-IDF. Only after audit, target build/QEMU. Do not repeat already completed host/ISA tests without changed inputs.

---

# 2026-11-27 — Atomic admission and buffer ownership, offline integration verified

- Exact isolated source 09ecbaca863f6e0e0c4b21a0e68a9ce41af5ac6a, parent 56171093142fb762316adc0bd3744c823b6b2064 (PR #44). New unlinked files only: src/room_admission.h a966c079a8ce399832d713a2c0d86c8465f0afc7; tests/test_room_admission.c 3aeb3ffdc9233f142a45e27e05b2758df3532360; tests/test_room_admission_sdk.c dd38564ea46ce6bb05cada928bc5e66f06942358. Git blob hashes match the three tested files. Ten existing channel/worker/drain/mock inputs reconstructed from repository and independently hash-matched to parent. Compare confirms no existing executable, dependency, CMake, workflow or configuration changes.
- Actual isolated Linux: GCC Debian 14.2.0-19 and Clang 17.0.0, strict C11 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -pthread, timeout 15 s. Four executed programs exit 0: admission 1361 assertions per compiler, actual channel/worker integration 1052 assertions per compiler (16 cases); 4826 assertions total. Admission checks generation, 32 concurrent lease capacity, duplicate leave, permanently closed gate, last release publishes wiped buffer, 100 four-thread enter/close iterations. Real room_phone_channel.c and room_sdk_stop.c linked to offline pthread SDK/clock mocks: callback and buffer leases independent, no new lease after closure, no borrowed reply release while work remains, both completion orders, partial start, duplicate close, SDK error, timeout and late completion. Outer fixture starts budget at 100 ms, backend first closes at 700 ms, both drain and SDK retain absolute 5100 ms deadline. No resetting inner budget in fixture. No new ESP-IDF runtime proof.
- Reproduce: compile tests/test_room_admission.c with flags above and -Isrc; second program add -Itests/browser_worker/mock_channel -Itests/mock_sdk_stop -Isrc main/room_phone_channel.c main/room_sdk_stop.c tests/test_room_admission_sdk.c; run each on both compilers with timeout 15. Actual command exit codes recorded. ASan/UBSan do not establish absence of data races.
- Design: atomically close admission and count leases; caller must initialize before publication and never reopen or copy a lease; ownership transfer must be externally synchronized. Context must outlive even denied callbacks until callback source stops. Bounded CAS can reject during contention. Limit 32 simultaneous leases, chosen experimentally, not an approved runtime limit. Requires 32-bit lock-free unsigned atomics; actual ESP32-C3 compatibility and stack/heap effects unverified. No real backend binding, actual Wi-Fi owner/session deadline propagation, TLS, hardware, OTA or operational activation. Existing protected code unchanged.
- Security gate: public experimental source has no private inputs, secrets, new workflow or network code; reviewed exact diff and main workflow triggers (manual or main-specific paths only), candidate workflow directory returns 404. Active ruleset protects original wifi-policy branch. Branch-protection read 403 and Actions permissions read 400; repository-wide admin/dependency audit incomplete. Local git transport DNS failed (exit 128); Docker, ESP-IDF and QEMU executables absent. New target CI deliberately not started pending audit and target atomic qualification. No new runner expense requested. Preserve original branches; rollback by abandoning draft or reviewed revert.
- Next: qualify atomics and actual gate lifecycle on pinned ESP-IDF; design real local backend callback source/lease transfer with safe lifetime, propagate one deadline through actual Wi-Fi owner/channel/session APIs and run regressions; only after required audit, isolated target build/QEMU. No protected merge or live network activation before gates.

---

# 2026-11-27 — Offline shared cleanup deadline and actual channel/worker integration

- New isolated experimental source 9cfb3010bfb7899f6aa5353ed79a8753b14c5b1f, based on combined branch 03f17e5a7fbf40b0131f3952e567df194a6697d9; original combined candidate retained. Three new files only: src/room_drain.h (7a5935ec5420f61f153723756fe6f2e22f6a7c78), tests/test_room_drain.c (679cc336e3d86258a392104b72a416812d4816ce), tests/test_room_drain_sdk.c (ca1f05e50d0205ce37a02673a5e3618ba9e1a1ec). All three Git blob hashes match the tested local files before publishing the new branch. Nine existing channel/worker and mock SDK input blobs matched pinned source 9d3f99cd3a5bf894007973cc216855e96aded73a. No existing code, CMake, workflow, dependency or configuration change; only offline model and tests, no production linkage.
- Actual isolated Linux tests: GCC Debian 14.2.0-19 and Clang 17.0.0, strict C11, ASan+UBSan, no sanitizer recovery, pthread, timeout 15 s. Four executions exit 0: deadline model + actual room_phone_channel.c 581 checks per compiler (26 fixture cases plus direct boundary checks); actual room_phone_channel.c + room_sdk_stop.c with pthread/FreeRTOS and SDK stop mocks 280 checks per compiler (16 fixture cases). Both completion orders, concurrent synthetic observations, partial open, repeated close, SDK error, deadline boundary, stale window, reverse clock, overflow edge, and no release after timeout/error. SDK worker code actually executes in second fixture; SDK function and timer are simulated, not a new ESP-IDF/QEMU or hardware test. ASan/UBSan are not ThreadSanitizer or proof of race freedom.
- Reproduce from source root: for each cc=gcc,clang compile with -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -pthread -Itests/browser_worker/mock_channel -Itests/mock_sdk_stop -Isrc. First link main/room_phone_channel.c tests/test_room_drain.c; second link main/room_phone_channel.c main/room_sdk_stop.c tests/test_room_drain_sdk.c. Execute each under timeout 15. No new Actions or network operation needed for these tests.
- Model owner-only, fixed absolute deadline supplied at begin, maximum 5000 ms, generation check, sticky lock on late/ambiguous result; actual callback admission, lifetime of retained buffers and propagation of deadline through live Wi-Fi/session teardown are NOT yet implemented. The isolated fixture connects actual channel to actual worker through a mocked backend. No operational networking, credentials, protected merge, hardware, OTA or production changes. Existing full administrative/dependency/history audit remains open. Rollback by abandoning candidate or reviewed revert, not history rewrite.
- Next: establish real local admission/ownership contract and propagate the same absolute deadline from the outer Wi-Fi owner through the channel/session teardown; test actual callback and buffer lifetime under pinned ESP-IDF, then target build and isolated QEMU; no live backend activation before review. Do not repeat the completed offline fixtures without a relevant change.

---

# 2026-11-27 — Read-only integration contract review (design gate)

- Read exact candidate 9d3f99cd3a5bf894007973cc216855e96aded73a: room_sdk_stop.c/.h, room_phone_channel.c/.h, room_apsta_task.c/.h, room_apsta_session.c, room_wifi_apsta_link.c/.h, room_wifi_idf.c/.h, room_network.c, main.c, main/CMakeLists.txt and existing owner/worker and partial-open fixtures. Worker and channel attachment APIs are linked but no actual phone IO backend or setup binding is installed by normal startup. Existing fixtures use synthetic drain observations; the successful SDK QEMU fixture is not a production callback/buffer ownership proof.
- Integration gate: a trusted local, boot-lifetime backend must close admission before ownership transfer, independently prove no application callbacks or retained buffers remain, and never release the radio or security context before the channel confirms drain. A failed partial open can still own resources; duplicate close must not repeat SDK destruction. SDK request failure must preserve explicit ownership/quarantine rather than guess that the handle is safe to reuse. No remote fields, timer-only proof, or SDK return alone may establish application drain.
- Timing concern from code inspection (not a reproduced runtime defect): outer Wi-Fi cleanup has a 5 s deadline, session cleanup starts its own 5 s clock after channel drain, and SDK worker takes a separate absolute deadline. The existing room_phone_io.close signature carries neither generation nor deadline. Before integration, specify one shared absolute cleanup budget through the local layers, check expiry after potentially slow operations, and keep sticky quarantine on any ambiguous or late result. Do not extend the outer budget by restarting an inner timer. Test both completion orders, partial open, duplicate close, clock/deadline boundaries, stale window, SDK errors and application drain delayed past SDK return in an offline fixture before any actual binding.
- Next concrete development step: isolated admission/drain and single-deadline contract model and regression tests, then review against pinned SDK behavior; no operational backend or protected merge before security gates. This entry is a source review, not a new compilation, runtime test or certification. No network activation, credentials, hardware, OTA or production changes.

---

# 2026-11-27 — Combined candidate CI and isolated SDK worker runtime verified

- Exact source 9d3f99cd3a5bf894007973cc216855e96aded73a, prior journal HEAD 3289f7341a8dcc241e0abbdaebb2dd71e821e42c; draft PR #42. CI-only draft PR #43 head ca65e1f660341aac839784ed281709c95d9a8b3c, workflow blob 0059e1cf2a91f8c9eae55d215f01d9c036cacada. Run 45957603730 / job 140367609550 completed SUCCESS; all four steps success. Full log re-read (452874 characters, 3545 newline-separated lines). Metadata read only; permissions {}; pinned inputs and SDK ESP-IDF v6.1.1; TEST_INPUT_BLOBS_OK (87 pinned input entries), seven HOST_TEST_OK, six SDK_STOP_HOST_OK (69 checks), 1030/1030 candidate build steps, GENERATED_TABLE_MATCH_OK and COMBINED_SDK_BUILD_OK. Workflow enforces partition/config/symbol checks. App 0xe46a0 bytes (935584), 2 MiB factory at 0x120000 within 4 MiB flash. Logged app SHA256 c56a3d21f1b593f43b28c3fda82ed0f7eeb68dd2cedaa21a269df640f7c3a061 and partition SHA256 b7e7cc4ebfcb5ce7d928eb74f9aa45866890d09399e7b9c44f3b0fa38cb37246. No retained artifacts; binary bytes not independently downloaded.
- Separate actual FreeRTOS/SDK loopback probe: 1018/1018 build steps, SDK_WORKER_RUNTIME_INPUT_OK; windows 1/2 normal, five polls each, freed=1 handoff=1; window 3 late, 105 polls, freed=1 handoff=0. SDK_WORKER_RUNTIME_PASS and QEMU_SDK_WORKER_OK. This is an isolated fixture, not a production network, TLS or hardware test. CMake emitted SDK component private-include warnings and git metadata lookup warnings in exported trees; no failed step. No new CI runs or deployment requested.
- Prior successful PRs #37, #40 and #41 remain open, separate source lines retained. Compare confirms both parents included and only three journal files differ between tested source and prior journal HEAD. Do not duplicate completed builds. Full administrative/dependency/history audit and physical tests remain incomplete; no protected code merge or operational activation.
- Next: review and specify the application admission/drain contract, then isolated integration of the inactive worker with the actual channel only after safe design and tests; preserve exclusive ownership, late quarantine, bounded deadlines and verified independent drain. No real credentials, listener activation, hardware, OTA or production changes. Rollback by reviewed revert; preserve all prior evidence.

---

# 2026-11-27 — Combined candidate build and actual worker runtime started

- Reconciled candidate 9d3f99cd3a5bf894007973cc216855e96aded73a, draft PR #42, parents 313457f5fc05c8e1410d32a90411b18cbdc70eaf and 22f8ef55876a94ec3936ecd60a7c4afd3486f21a. Exact 170-file tree verified against union of reviewed changes, all ten non-document changed blobs preserved, journal prefixes combined without duplicating common history. Separate original branches retained. No operational activation.
- CI-only draft PR #43, head ca65e1f660341aac839784ed281709c95d9a8b3c, workflow blob 0059e1cf2a91f8c9eae55d215f01d9c036cacada. Run 45957603730 queued at first check. One public standard ubuntu-24.04 runner, 20 min cap, permissions {}, restricted same-repo PR head/path, pinned SDK image/commit and QEMU archive hash. No external Actions, credentials, deployment, artifacts or live network. Two offline container steps, 87 exact input blob checks, seven channel/task programs and six worker scenarios, full ESP32-C3 candidate build. Separate isolated QEMU probe compiles exact worker source/header and invokes actual SDK stop via a test callback: two successive normal windows, third late window, independent fixture buffer wipe/destructor, heap integrity, owner progress and sticky quarantine. No TLS, hardware or production backend drain proof. Workflow derived from verified templates and re-read; no new runtime PASS yet. Full administrative/dependency/history audit remains incomplete.
- Next: verify all steps/full logs, seven HOST_TEST_OK and six SDK_STOP_HOST_OK, COMBINED_SDK_BUILD_OK, SDK_WORKER_RUNTIME_INPUT_OK, three SDK_WORKER_CASE records, SDK_WORKER_RUNTIME_PASS and QEMU_SDK_WORKER_OK. Confirm exact source and partition/config/symbol checks. Only then review trusted application drain/admission and actual channel integration. No protected merge, operational networking, hardware or OTA. Preserve original branches; rollback by reviewed revert.

---

# 2026-11-27 — Reconciled pretrial guard and inactive SDK stop worker

- Common ancestor 1735144220be563d05ff29d9a82a62bb3c315452; PR #38 head 313457f5fc05c8e1410d32a90411b18cbdc70eaf, tested code e98883e476bb335f915e79c5e3f50e3a86282d50; PR #39 head 22f8ef55876a94ec3936ecd60a7c4afd3486f21a, tested code 18da1451b78b35f4cc07f66516e932c92bcdc674. Both parents preserved in separate experimental merge commit. All ten changed non-document blobs transferred unchanged, no overlapping code paths, no activation or startup/configuration change. Both journal histories retained, common prefix recorded once. Earlier pending entries below superseded by verified results.
- PR #40 run 45954028587 job 140356222965, all three steps success; full log 1757 lines/228129 chars. 80 input blob checks; seven HOST_TEST_OK markers including pretrial fixture; PRETRIAL_ORDER_BUILD_OK, 1029/1029 target steps. App 934272 bytes, SHA256 ffd708e05bd95c3d5845233568c76832bfccfc5d5916cb7e0383bab7a44d0b0e, factory headroom 1162880 bytes.
- PR #41 run 45954145974 job 140356606863, all three steps success; full log 1733 lines/218972 chars. Eight changed blob checks; six SDK_STOP_HOST_OK scenarios normal 18, late 14, clock 14, error 13, invalid 8, alloc 2 (69 checks); SDK_STOP_WORKER_BUILD_OK, 1030/1030 target steps, all three new API symbols linked. App 935568 bytes, SHA256 dbed568316b9e11e5ec212f6845269e2030a3fe16fcaff17358fcf812bc6b163, factory headroom 1161584 bytes. Both runs SDK 6.1.1 GCC 15.2.0, partition SHA256 b7e7cc4ebfcb5ce7d928eb74f9aa45866890d09399e7b9c44f3b0fa38cb37246, five SDK CMake warnings each, metadata read only, zero artifacts; no independent binary verification. No new combined PASS claimed.
- Remaining: combined host/target regression, actual room_sdk_stop worker on FreeRTOS with real SDK HTTP stop in isolated QEMU, trusted backend admission and application drain, TLS, AP-only isolation, storage and hardware. Full administrative/history/dependency audit incomplete. No protected merge, deployment, real network or equipment. Rollback by reviewed revert, preserve both original branches.

---

# 2026-11-27 — Pretrial ordering reconciliation and regression started

- Reconciled current experimental base 1735144220be563d05ff29d9a82a62bb3c315452 against PRs #17/#21/#22/#23/#24 at fixed heads. #17 and #23 are ancestors (69 and 37 commits ahead, respectively); #21 and #22 each diverge by one commit (69 ahead), #24 diverges by one (68 ahead). Deadline and DHCP checks present in current code; recovered explicit pretrial ordering guard from #24 instead of merging diverged branches. Current code/test changes only main/room_apsta_task.c da730a837fb5f176f90a1bb5c585e1f719f72315 and tests/test_room_apsta_task.c a5327b64373ebdb8af5b0456d4affc54185c7c3c, commit e98883e476bb335f915e79c5e3f50e3a86282d50, draft PR #38. New synthetic default-loop post failure regression; no local execution claimed. Direct Git fetch failed DNS; connector available.
- Separate CI-only draft PR #40, head 2988f8f8f9f3262306bb58a3a21d76e0d0392bb1, workflow blob 45d5b0d3bb5469678160d751b4f6b953e4d95b1b. Derived from previous successful pinned workflow. Explicit source SHA and 80 input blob checks, six channel regressions plus task regression, exact target build, partition/security checks. Standard public runner, permissions {}, offline container, fixed SDK image, no secrets, hardware, deploy or artifacts. Full administrative/history audit incomplete. Run 45954028587 / job 140356222965 in_progress at capture; setup succeeded; no new PASS claimed.
- Concurrent PR #39 appeared between #38 and #40, branch experiment/sdk-stop-worker-20261127, head 18da1451b78b35f4cc07f66516e932c92bcdc674, same base. Its eight changed paths are distinct from #38's two paths. Read its worker source/header; no duplicate implementation or merge. Its claimed mock PASS is not independently verified here; target build pending. This is confirmed concurrent repository activity, not proof of which session/actor performed it. Common branch and private journals intentionally not written during concurrent development; this isolated branch records only its own checkpoint.
- Next: inspect run 45954028587 terminal status/full logs and separately verify #39 source/test/CI evidence; re-read heads and journals, reconcile non-overlapping candidates with reviewed combined tests. No startup, protected merge, live network, OTA or equipment operation. Roll back isolated changes by reviewed revert, not history rewrite.

---

# 2026-11-27 — Inactive persistent SDK stop worker; target build pending

- Candidate source 18da1451b78b35f4cc07f66516e932c92bcdc674, public draft PR #39. New main/room_sdk_stop.c blob 329521c22855caaffe41753b1dcf34e4b08ed29d, src/room_sdk_stop.h blob 6c94a54f9f51f8cb767d51b2cc1b339964ec1202, test blob e97fb7a025b22f82244e5d8b26acd89b6686ec77. CMake blob 03c90f47705ae1084f5ce48bb8e0493f8dd97116 retains three uncalled APIs in the target link; no normal startup invocation, actual server or network binding. One boot-lifetime worker, task notification and release/acquire publication; exclusive owner, monotonic window and absolute deadline bounded to 5000 ms. SDK failure, clock rollback or timeout quarantine; no reuse after late completion. Independent application drain proof is required from future trusted backend; no implementation of that proof or of operational transport in this change.
- Actual local GCC 14.2.0 and Clang 17.0.0 strict C11 -Wall -Wextra -Werror -pedantic -g -O1 -pthread -fsanitize=address,undefined -fno-sanitize-recover=all -fno-pie -no-pie, six executions per compiler (normal 18, late 14, clock 14, error 13, invalid 8, alloc 2; 69 checks per compiler, exit 0, each timeout 10 s). Initial compile failed on missing NULL declaration; added stddef.h and reran both compilers. All seven local file Git blobs matched uploaded source/test/mocks. Reproduce with -Itests/mock_sdk_stop -Isrc main/room_sdk_stop.c tests/test_room_sdk_stop.c. Mock SDK stop and logical timer; real pthreads, not target FreeRTOS execution.
- Branch reconciliation: PR #17 and #23 ancestors of previous candidate 1735144220be563d05ff29d9a82a62bb3c315452; #21/#22/#24 divergent. Existing reply timeout, DHCP status and event barrier present, but a missing pretrial post-failure guard is addressed by separate PR #38 head e98883e476bb335f915e79c5e3f50e3a86282d50, two changed files main/room_apsta_task.c and tests/test_room_apsta_task.c. Parallel PR #40 head 2988f8f8f9f3262306bb58a3a21d76e0d0392bb1, run 45954028587 in progress at inspection. Neither created in this session; no overlapping files with #39 delta, no wholesale branch merge or shared baseline modification. Must reconcile verified changes before integrated candidate.
- CI-only draft PR #41, workflow commit f1f04444fb014f01284e31efac154a388c8a26e4, workflow blob fcf1d540af049315731ee9ea7e14bc67121e5007, run 45954145974 in_progress at capture. Prior verified CI template with exact source SHA and eight changed blob checks, public runner ubuntu-24.04, 20 min limit, permissions {}, same-repo/branch/path restriction, pinned SDK image and SDK commit, offline capability-dropped container, no secrets/artifact/deployment. Target compile and link of full inactive candidate and six host scenarios pending; no new target PASS claimed. Full dependency/history and administrative audit incomplete.
- Next: inspect #41 terminal status, all steps/full log, six SDK_STOP_HOST_OK markers, SDK_STOP_WORKER_BUILD_OK, symbol and partition/config checks. Independently verify #40 and reconcile tested non-overlapping guard/worker changes; then execute new worker itself with real SDK/FreeRTOS in isolated QEMU, prove trusted backend admission/actual drain and deadline before attaching to channel. TLS trust, AP-only isolation, protected storage, MQTT and hardware gates remain. No protected merge, operational activation or flash. Revert reviewed experimental commits for rollback.

---

# 2026-11-27 — Actual SDK stop probe PASS

- Public CI-only draft PR #37, head e120c3157e4f777ce16b9ad4f913faa30315b6c4, workflow blob ce937adde9e1696abc2db4a61e198c7aa24c8029 re-read. Run 45950866026 / job 140346141225 completed success at 06:20:12 UTC; all three steps successful. Full job log retrieved (1788 lines, 222032 characters). Actual terminal markers SDK_STOP_CASE late=0 polls=5 freed=1 handoff=1; SDK_STOP_CASE late=1 polls=105 freed=1 handoff=0; SDK_STOP_RUNTIME_PASS; QEMU_SDK_STOP_OK. The emulator returned from app_main before the expected timeout termination. No runtime assertion or compiler error marker found in reviewed output.
- Pinned ESP-IDF 6.1.1, target GCC 15.2.0, QEMU 9.2.2, verified image digest e5b767c64fdfdf427aa14cc7aaa15d24caf2f22c78241c5a3ed18150cfa7f05c; input probe.c SHA256 8c6b4aa14c3aedfd2528cb789e5160e897c3c84890b0025f559813b5416e7554, main/CMakeLists.txt b39ecae3f2ea482b9da8895c1c3e3ec6639393ba431aad56aa2f85cb8501e877, sdkconfig.defaults 3a63feb295ddffa897175d3e40508e7eb2deccce8a5c8ec7d46562e1a39d8645 from log. Build completed, image size reported 308937 bytes. Five SDK CMake dependency warnings involving esp_wifi/wpa_supplicant. Metadata: read; zero artifacts; no independent binary verification. No repeat CI needed.
- Actual HTTP server queued callback and stop path executed in NIC-less QEMU, separate FreeRTOS stopper, synthetic buffer wipe and context destructor, heap integrity; normal and late fixture handoff outcomes confirmed. This is not actual HTTPS/TLS, production room_wifi_idf.c deadline, AP/STA isolation, partial-start failure or hardware timing proof. No production code/configuration changes, physical network access or deployment. Full dependency/history and administrative settings audit incomplete.
- Next: review remaining SDK lifecycle gaps and existing divergent experiment PRs before integrating an inactive, bounded worker candidate. Retain authentication, browser trust, storage, AP-only isolation, actual deadline and hardware gates; verify failure paths and compile exact candidate with pinned SDK after audit. PR #37 remains open, unmerged for evidence. No protected code merge, operational networking or flash. Rollback experimental additions by reviewed revert.

---

# 2026-11-27 — Actual SDK stop probe started (result pending)

- Reviewed pinned ESP-IDF 6.1.1 source 051e4c4eb20346e043cdd47bc388fde75ff8af63, httpd_main.c blob a273b3bab0fcb4ee2aa0a8cf44bfbd3810effbb0 and control socket blob 74e09bd2edc5937c2ca037cbd5c3a8c817fedb24. The control socket requires loopback; queued work executes in the server task, httpd_stop waits for THREAD_STOPPED before context destruction. Prior QEMU run 45932541494 / job 140288229299 independently re-read: ACK_RUNTIME_PASS, QEMU_ACK_RUNTIME_OK, 128 exchanges; that is prior adapter evidence, not server evidence.
- Separate CI-only draft PR #37, branch ci/sdk-stop-probe-20261127, workflow commit e120c3157e4f777ce16b9ad4f913faa30315b6c4, blob ce937adde9e1696abc2db4a61e198c7aa24c8029, SHA256 a427eb631c16a1b5383a1ba8ece611e076feafd240f8965546e52eff5b20587e. Local PyYAML and outer/inner Bash syntax checks passed; published blob matched. Compare with main d8d613fa09b39c26296e86e7963d49d6dc63ba17 showed one new workflow only. No production or firmware source changes.
- Synthetic standalone project uses actual ESP-IDF HTTP server, httpd_queue_work and httpd_stop in separate FreeRTOS task, no URI handlers or external interface, SDK loopback control only. Normal and late cases exercise real emulated timer, callback buffer wipe, custom context destructor, heap integrity and fixture-only quarantine. No TLS handshake, actual Wi-Fi owner, partial-start failure or physical hardware proof. Do not infer a working browser backend or device deadline from this probe.
- Standard public runner, same-repository PR and exact branch/path trigger, permissions {}, 12 min limit; no secrets referenced, no checkout of private files, no artifacts/deployment; pinned SDK image and QEMU archive hash from previous verified run, offline capability-dropped container and QEMU -nic none. Administrative secret/settings and full dependency/history audit remain incomplete.
- Run 45950866026 / job 140346141225 in_progress at capture, setup step successful. Job logs unavailable while running (HTTP 404 BlobNotFound). No new build or runtime PASS claimed. Next: inspect terminal status, every step and full log for actual compile and both SDK_STOP_CASE results, SDK_STOP_RUNTIME_PASS and QEMU_SDK_STOP_OK; resolve failure if any before integration. Do not duplicate an active run, merge PR #37, enable networking or flash equipment.

---

# 2026-11-27 — Offline owner/worker shutdown integration

- Isolated host-only test tests/browser_worker/test_owner_worker_shutdown.c uses unchanged room_phone_channel.c f3e3910d5b6cd45d497e8988aab45555a258d45e, room_phone_channel.h c19526cb359f85ebf35e3dc1fd3aae7dc4527a3e, mock room_apsta_task.h be40b2441aa79b980c868fd79981d3971ed5bfd1, room_wifi_owner.h c0c5f6d82dc195a71349b925a6bbd63d6967c0f8 and room_wifi_policy.h c54bd7ee5916dbc050e4199dce58bb10d2000e06; all five locally verified Git blob hashes. Actual GCC 14.2 and Clang 17 strict C11 ASan/UBSan, each exit 0 with 102 checks across four scenarios and 10 s timeout. Compile main/room_phone_channel.c and test with -Itests/browser_worker/mock_channel -Isrc -std=c11 -Wall -Wextra -Werror -pedantic -g -O1 -pthread -fsanitize=address,undefined -fno-sanitize-recover=all -fno-pie -no-pie. Test blob a9a22c4039f2ddad1420b54230d1534a8b453a77, SHA256 85b3ceb5fe751737a0b98cb02231ad31ab4f1d463e840056d8d3afab719491ff.
- Actual separate pthreads and semaphore stop request; owner progresses while mock worker blocks. Supervisor joins both threads outside owner; both completion orders tested, no STA restart before both finish and synthetic buffer wipe, and no restart after modeled 5 s timeout even on late completion. The deadline and SDK operations are mocked; not a test of room_wifi_idf.c, httpd_ssl_stop, real callback lifetime, actual timing or hardware. Previous partial-open binaries also rerun successfully (138 checks per compiler). No production code or dependency changed.
- Local ESP-IDF, target toolchain, Docker and QEMU absent; Git direct access failed DNS, GitHub connector available. No target build attempted for this host-only test. Existing firmware source unchanged, previous build evidence retained. No new workflow or automatic run intended. Full dependency/history and administrative audit remains incomplete. No network, secrets or device access.
- Next: qualify an isolated actual ESP-IDF lifecycle test without activating operational networking. Verify SDK stop/callback/buffer behavior and real adapter deadline, then review integration and target build. Do not merge protected code or flash hardware. Rollback by reviewed revert.

---

# 2026-11-27 — SDK shutdown boundary review and offline partial-start regression

- Reviewed exact ESP-IDF 6.1.1 source 051e4c4eb20346e043cdd47bc388fde75ff8af63: httpd_main.c blob a273b3bab0fcb4ee2aa0a8cf44bfbd3810effbb0, https_server.c blob 16af012c2cb6c4ee563a7aa23c3891b9a5e8e856. httpd_ssl_stop delegates to httpd_stop, which takes a semaphore with portMAX_DELAY and waits for THREAD_STOPPED before freeing server contexts. A stop event alone is not an application callback or buffer drain proof. Keep blocking SDK work outside the Wi-Fi owner. The existing owner has a 5 s outer cleanup deadline and quarantines failed cleanup; timeout is not resource release.
- New standalone tests/browser_worker/test_partial_open.c: actual channel source f3e3910d5b6cd45d497e8988aab45555a258d45e, header c19526cb359f85ebf35e3dc1fd3aae7dc4527a3e and mock be40b2441aa79b980c868fd79981d3971ed5bfd1 matched GitHub blobs. GCC 14.2.0 and Clang 17.0.0, strict C11 -Wall -Wextra -Werror -pedantic -g -O1 -fsanitize=address,undefined -fno-sanitize-recover=all -fno-pie -no-pie, timeout 10 s, each exit 0, 138 checks per compiler. All six permutations of synthetic stop return, callback drain and buffer drain after partial open; no reopen, polling or foreign-owner close before cleanup, no repeated backend close after success. Test blob f4875191bb26bf5d9a3bf8eeab30d7b647690099, SHA256 6f03cdc9fd779f6143494fe093f8c7a95dfbe5d2fbd8d39463eb1b198ad78c70. Reproduce by compiling main/room_phone_channel.c and the test using -Itests/browser_worker/mock_channel -Isrc plus the above flags. Simulated flags are not SDK observations.
- Base 3de1f62cae838347ec98cd46a2f31054e5158a3f; no firmware, dependencies, workflows, startup, network or device changes. Main workflows inspected for triggers; no matching automatic test trigger for this candidate. Administrative and full dependency audit incomplete. Prior exact target build remains valid for unchanged source.
- Next: establish an isolated SDK lifecycle fixture that observes actual SDK stop return and callback/buffer quiescence without blocking the Wi-Fi owner, including partial start and timeout quarantine. No real network activation until browser trust, authentication, protected storage, interface isolation and hardware gates are satisfied. Do not merge protected code. Rollback candidate by reviewed revert, not history rewrite.

---

# 2026-11-27 — Channel close regression and exact target build PASS

- PR #34 closed without merge at head 63acfa28634f8843eb09c8cf4ee98e00ce8351d0, corrected workflow blob 4f764b2f2320662dd81b15f9c54af843732d5be3. Run 45947672649 / job 140335996989, all 3 steps successful; retrieved full 1732-line log (219807 characters). Exact source abe59e3c0e7f8e2c7099baf1ed34d8198dcdaa9e; ten input blob checks; four actual GCC ASan/UBSan regressions PASS: 1823 checks, 1000 two-thread exchanges, four stop/expiry scenarios with 56 checks, 29 close-retry checks. ESP-IDF 6.1.1 / target GCC 15.2.0; 1029/1029 target build steps; generated partition table matched, five required symbols and default configuration checks passed. App 934256 bytes, factory headroom 1162896 bytes, static DRAM 112812/321296. App SHA256 12461468c022ca83be7f37d463371167a5951cafb0202eefd564f64b3c081443; table SHA256 b7e7cc4ebfcb5ce7d928eb74f9aa45866890d09399e7b9c44f3b0fa38cb37246. Five SDK CMake warnings; no compiler error marker in reviewed output. Metadata read, zero artifacts, no independent binary download. Initial failed run 45947440581 remains historical; retry corrected feature macro. No repeat CI or QEMU. Before this entry, comparison with tested source showed nine later commits modifying only the three journals.
- Next: review integration of channel lifecycle with real SDK worker and prove actual shutdown/drain before handoff; retain authentication, storage, physical timing and browser trust gates. No activation, hardware flash, OTA, heating changes, protected merge or production deployment. Revert experimental correction only by reviewed revert if needed. The green build does not establish a functioning real network transport.

---

# 2026-11-27 — CI feature macro correction; retry started

- PR #34 run 45947440581 / job 140335266736 completed failure. Full log 249 lines, ten exact inputs verified, pinned SDK ESP-IDF 6.1.1 and cross GCC 15.2.0 confirmed. Host GCC 13.3.0: test_channel_ack passed 1823 checks; compiling test_channel_threads failed because forced mock include brought in glibc _POSIX_C_SOURCE=199506L before test definition 200809L. Target build was not reached; no other host PASS claimed. This is a compile-command issue, not evidence of a runtime failure.
- Fixed only CI compiler flag -D_POSIX_C_SOURCE=200809L, preserving -Werror and ASan/UBSan. Local GCC 14.2 and Clang 17 feature-macro probe compiled/ran, actual close-retry fixture again passed 29 checks on each after correcting missing local mock include path. YAML and outer/inner Bash syntax checked. Published workflow head 63acfa28634f8843eb09c8cf4ee98e00ce8351d0 / blob 4f764b2f2320662dd81b15f9c54af843732d5be3 matches local; one-line diff, exact source unchanged abe59e3c0e7f8e2c7099baf1ed34d8198dcdaa9e, security controls unchanged. Retry run 45947672649 in_progress at capture. No merge/deployment or device changes. Next inspect terminal result and all logs, no duplicate rerun while active. Retain failed run as evidence.

---

# 2026-11-27 — Affected channel regression and target build started

- PR #34 is an isolated CI-only draft; workflow head 3bb56040a045b2e6bfb59299b23b158ed3bbfa39, blob 402faddc9003bc90a6e88571f3166b2787aaadc4 matches locally checked YAML. Outer and inner Bash syntax pass (initial inner extraction error corrected before publication). Source fixed at abe59e3c0e7f8e2c7099baf1ed34d8198dcdaa9e, ten explicit Git blob checks, four related offline C host regressions then exact ESP32-C3 build. Same-repo branch/path restriction, standard public ubuntu-24.04, permissions {}, 20-minute cap, pinned SDK image previously verified, no network in build container, no artifacts, secrets or private inputs. Existing main workflow triggers checked, no overlap. Full administrative and historical/dependency audit limits remain.
- Run 45947440581 / job 140335266736 observed in_progress, setup step successful; no new host-regression or target PASS claimed yet. Inspect full logs and exact source checks after terminal status; close test PR without merge when reviewed. No repeat of unchanged prior QEMU probes. No production activation, deployment, OTA, physical tests or protected merge.

---

# 2026-11-27 — Offline phone-channel close retry correction

- Actual channel code audit identified a defensive cleanup issue: after backend close returns true, a later retry can call the already closed backend again if owner-side cleanup has not finished. Candidate latches successful backend drain for the window, does not release a pending response during dispatch, retains owner/window checks, and does not report failed sends as success. This is a synthetic fault-path finding, not an observed physical SDK failure; no network backend or activation added.
- Baseline d38ab044c569b811ca57a1f12e4ef2817e8cdd2c. Exact original source/header/mock Git blobs ce7369f90833bd796ed2dab0b11579c637f6b487 / ef0c5d44c5a32ac190b71fb38000da876d8ad618 / be40b2441aa79b980c868fd79981d3971ed5bfd1 matched local copies. New offline test fails against baseline on GCC 14.2.0 and Clang 17.0.0 (exit 1, same retry assertion); corrected source passes 29 checks on each (exit 0). Strict C11, -Wall -Wextra -Werror -pedantic -g -O1 -fsanitize=address,undefined -fno-sanitize-recover=all -fno-pie -no-pie; compile main/room_phone_channel.c and tests/browser_worker/test_channel_close_retry.c with -include tests/browser_worker/mock_channel/room_apsta_task.h -Isrc, timeout 10 s. Local execution used identical mock header on include path. Covers rejected non-owner close, busy close, retained drain, delayed finish, dispatching guard and stale window. No actual SDK, socket, credentials or hardware.
- Source checkpoint abe59e3c0e7f8e2c7099baf1ed34d8198dcdaa9e; tested Git blobs source f3e3910d5b6cd45d497e8988aab45555a258d45e, header c19526cb359f85ebf35e3dc1fd3aae7dc4527a3e, test 72627fce588769277f17b582c3bad1b9d94aa3b9; uploaded bytes checked. SHA256 source d02c0f1d2a590042f2857275f5b6d65ee6793ae14ef17e874e88f827150c2a87, header fbfb05490a09fe4b0c5e88c710c80d9a42a669c5527ea5775c3355902f539cb4, test da2634ab787c236cb798a63a4b34a3856dcd8076247b342380d8a351bb48db99. Isolated branch has no workflow directory; no new dependencies, private data, configuration, deployment or OTA changes. Prior full history/dependency/admin audit limitations retained.
- Next: run existing affected channel regression suite and target build for corrected source, then integrate actual bounded backend only after trust/storage and resource shutdown design are reviewed. Prior PR #33 applies to older exact source, not this correction. No stable release or real Wi-Fi proof. Revert the isolated code/test changes or use unchanged base if needed.

---

# 2026-11-27 — Exact committed ESP32-C3 candidate build PASS

- PR #33 closed without merge after run 45944680171 / job 140326558444 completed successfully; all three steps, full log 234129 characters / 1793 lines. Source c316fa8ffbb6f168463f7ac8ecda51b736f49ff4, workflow 582956eac7fbdfb8c191733c4816be681f4e143b / blob 94ce5de331d0dbb0f6e3211e9b3db28fb9016638. ESP-IDF 6.1.1, GCC 15.2.0. BUILD_EXIT=0, COMMITTED_CANDIDATE_BUILD_OK, CANDIDATE_BUILD_OK; generated partition matches approved CSV and flash offset 0x120000. App 934224 bytes, factory headroom 1162928 bytes, static DRAM 112812/321296 bytes. Partition SHA256 b7e7cc4ebfcb5ce7d928eb74f9aa45866890d09399e7b9c44f3b0fa38cb37246; app SHA256 ac16cf7d3ba0dfcf3014b9a1aeb7e715b5077c7f096d259b4c779f5f2be7c2e5 as logged. Four required linked symbols and default safety configuration checks passed. Token Metadata read; zero artifacts, no independent binary download. No compiler warning/error markers found; full administrative/dependency audit remains incomplete.
- Next: review real bounded phone backend, owner-only Wi-Fi handoff, actual worker exit/drain and secure browser trust/storage; integrate only after review and test. Default networking remains disabled; no actual network, hardware, flash, OTA or protected code merge. No duplicate CI for unchanged source. Prior pending entry is historical.

---

# 2026-11-27 — Exact committed ESP32-C3 candidate build started

- Integrated build inputs: all 48 paths independently compared with prior qualified source 75df361f733973bd7d58e7bd856af96da28bdbb6; only approved main/partitions.csv differs (blob 465649e490ecb7fb04a459f38430dd5bf00614a4). Current source c316fa8ffbb6f168463f7ac8ecda51b736f49ff4. The actual room_network.c starts only the valid-profile STA path; missing profile does not authorize AP and experimental phone link is not activated. HMAC key ID -1 keeps networking disabled in default candidate. Tests/browser_worker are not linked into firmware.
- PR #33, ci/candidate-build-20261127, workflow head 582956eac7fbdfb8c191733c4816be681f4e143b, workflow blob 94ce5de331d0dbb0f6e3211e9b3db28fb9016638. Run 45944680171 in_progress at capture. No PASS claimed. Local PyYAML BaseLoader and bash -n on outer and inner scripts passed; published Git blob exactly matched local syntax-checked bytes. Only one isolated workflow added, pinned public source and ESP-IDF 6.1.1 image, same repository branch/path-restricted pull_request, permissions {}, standard public runner, 20 min cap, offline capability-dropped build container, no artifacts or source edits. Three existing main workflows' triggers checked; no other PR CI matches. No private inputs or credential references, full administrative and dependency audits remain incomplete.
- Next: inspect actual completed build, full log, generated partition SHA256 b7e7cc4ebfcb5ce7d928eb74f9aa45866890d09399e7b9c44f3b0fa38cb37246, flash manifest factory offset 0x120000, binary size/headroom, linked symbols and safety config. Close test PR without merge. Do not repeat QEMU model or run #33 merely for another status. This target build cannot establish live browser transport, actual Wi-Fi handoff or physical hardware safety. No activation, device writes, OTA or protected code merge. Subsequent work: real bounded phone backend and end-to-end integration, subject to trust/storage/hardware gates.

---

# 2026-11-27 — Offline completion adapter runtime PASS

- Verified PR #32 source 8cd45dfb1e632e3709a2c51856cc31bcba44bfeb, workflow head 61ad6c2ce663ca21e0501fd512875b59981ea60a, run 45932541494 / job 140288229299: all three steps, 1018/1018 build steps, 88 checks plus 128 FreeRTOS exchanges and one expiry in ESP32-C3 QEMU. Full log 1860 lines; four input blobs matched, heap integrity passed, free heap returned to 318356 bytes, stack remaining 3248/3320. No artifacts. PR closed without merge; no rerun. Offline model only, not real network or firmware validation.

---

# 2026-11-27 — Pinned offline completion adapter runtime test under ESP32-C3 FreeRTOS/QEMU started

- Prior independent QEMU resource PASS checkpoint 8cd45dfb1e632e3709a2c51856cc31bcba44bfeb preserved. Local tool discovery: GCC and Clang available, no idf.py, qemu-system-riscv32 or Docker in PATH. New target execution uses four exact public blobs from the existing checkpoint: completion_adapter.h 47104a703fbae97821b0d163de594dc2a0eb8113, mailbox.h c73f0dd25a66edd063f30d314a40b590acebe859, worker_model.h bbb110d76bffede4942330ce7d59a99eca8af42d, test_ack_adapter.c 10ec3812a774cbafb9c761adf8edf44a51a98607. No changes to those components.
- Independent runtime harness calls the existing 88-check regression (main renamed at compile time; assertions enabled), then exercises one shared offline model from two FreeRTOS tasks, with 128 checked 4096-byte request/reply exchanges coordinated by queues and an additional expiry scenario using esp_timer_get_time and task delays 2100/2300 ms. Checks copied data, receipt consumption, rejected late completion and missing drain evidence, fixture task deletion and buffer clearing, heap integrity, free heap and stack high-water marks. This is queue-coordinated synthetic execution, not a proof of lock contention behavior or a test of the actual SDK/network channel. No actual runtime result yet.
- Draft PR #32, ci/ack-runtime-probe-20261127, head 61ad6c2ce663ca21e0501fd512875b59981ea60a, workflow blob 3ec6fd9a325ca14e9ea27ff1b83ee5905c93d812; only one added workflow from unchanged public main d8d613fa09b39c26296e86e7963d49d6dc63ba17. Reviewed exact published content, pinned prior verified ESP-IDF 6.1.1/GCC 15.2.0 image and QEMU archive SHA256, same-repo branch/path-restricted pull_request, permissions {}, standard public runner, 12 min cap, offline capability-dropped container and NIC disabled, four input Git blob checks, no external Actions, private inputs or artifact upload. Administrative audit and full current history audit incomplete; no claim of full audit. No device, firmware activation, OTA, production or protected branch changes.
- Run 45932541494 / job 140288229299 observed in_progress at capture. No PASS claimed. Next inspect completed run, all steps, full log, exact hashes, UART regression/expiry/exchange markers and actual resource metrics; distinguish emulator time from physical time. Close test PR without merge after verified result, then review remaining integrated firmware and hardware gates.

---

# 2026-11-27 — Independent FreeRTOS resource execution in ESP32-C3 QEMU PASS

- Run 45930476924 / job 140281684348, workflow head 32a42ba1f9f64908c9765a869cdf8de15acd1285, workflow blob f27bc73529fe1836ab4f7b3874fedb5f1e94a81f. Completed success, all three job steps successful. Full log retrieved (221178 characters, 1776 lines). QEMU archive checksum verification OK, actual QEMU 9.2.2 esp_develop_9.2.2_20260417, ESP-IDF 6.1.1 and GCC 15.2.0. Logged synthetic probe.c SHA256 fa05332d3988a97f343c257055ca86572e416790ed3df64bc8a35b91d1d6d209. 1017/1017 build steps and actual emulator boot into app_main, unicore scheduler, all 128 checked queue exchanges. UART RESOURCE_PROBE_PASS and QEMU_RESOURCE_RUN_OK; emulator stopped by expected timeout after app_main returned.
- Measured internal 8-bit free heap before tasks 327332 bytes, live and historical minimum 318556 bytes, after task deletion/idle reclamation and queue/semaphore deletion 327332 bytes, largest free block afterward 188416 bytes; heap integrity check passed. Dynamic difference 8776 bytes (includes task stacks/TCBs and queues/semaphore), static data buffers 8192 bytes already included in baseline. Two requested 4096-byte task stacks, reported high-water remaining 3752/3744 bytes (344/352 used at sample points). Emulated elapsed 27404 us, not physical timing. Single synthetic workload and matching final heap do not prove leak-free operation or establish full firmware memory/stack requirements. Linker reports DRAM 58265/321296 bytes and binary 0x25600 = 153088 bytes, separate from runtime heap metrics; do not add or equate different memory accounting scopes. Default standalone partition layout, not product layout.
- Five SDK CMake component warnings and six Kconfig choice-symbol notes; no compiler warning/error markers in reviewed log. Metadata read only, zero artifacts, NIC disabled and offline container; no physical device, real flash/OTA, Wi-Fi/TLS, actual phone channel or sensor runtime tested. QEMU simulated watchdog disabled by SDK helper; no watchdog validation. Administrative audit and current complete history audit remain incomplete. PR #31 closed without merge after review, source/logs preserved. No extra CI reruns.
- Next: execute the exact previously tested completion adapter and channel-related synthetic scenarios under FreeRTOS/QEMU, with measured stack/heap and bounded completion/cleanup, then assess actual firmware build and remaining hardware/trust/storage gates. Keep independent baseline evidence and distinguish emulator from physical measurements; no production activation or protected branch change.

---

# 2026-11-27 — Independent FreeRTOS resource probe in ESP32-C3 QEMU

- Prior model compile/link PASS checkpoint 0d8bb9ba4275d56af206e0430385a1090e5149d1 preserved; no repetition of earlier Linux tests or model compilation. Local environment rechecked: GCC 14.2.0 and Clang 17 available; qemu-system-riscv32, idf.py, riscv32-esp-elf-gcc, Docker/Podman absent; github.com DNS lookup unsuccessful. Espressif official ESP32-C3 QEMU and RAM usage documentation confirms emulator support and IDF stack high-water mark units are bytes. Exact SDK 051e4c4eb20346e043cdd47bc388fde75ff8af63 tools/tools.json blob cb3c9af548a60af831b01b6032d7e3e87b5e97bc pins qemu-riscv32 esp_develop_9.2.2_20260417 Linux amd64 archive size 16842920 and SHA256 547f03e04701a92cbb699f7f7d015adc1f5b5ef93cbb94c0dd9b7107e2d84e77; qemu_ext.py blob 6aa8a7a5fc8d6f599d6796ae683db707d6ea0d72 reviewed for idf.py qemu and NIC override behavior.
- Created independent synthetic resource workload (not actual phone channel or sensor firmware): two FreeRTOS tasks, 128 checked queue round trips, two static 4096-byte buffers, two requested 4096-byte task stacks, bounded queue waits, separate completion reports and self-deletion, idle reclamation interval. Report free heap before/live/after, lifetime minimum and largest block, task stack remaining, emulated elapsed time and heap integrity. No real time performance or leak-free guarantee inferred from a single synthetic sample. No hardware or network APIs in probe.
- Draft PR #31 branch ci/freertos-resource-probe-20261127, head 32a42ba1f9f64908c9765a869cdf8de15acd1285, workflow blob f27bc73529fe1836ab4f7b3874fedb5f1e94a81f. One workflow only from public main d8d613fa09b39c26296e86e7963d49d6dc63ba17; reviewed diff, verified exact uploaded content, checked shell quoting and key isolation constraints. Same-repo branch/path-restricted pull_request, permissions {}, 12-min standard public runner, no external Actions or private inputs; SDK image same pinned digest as successful PR #30, exact SDK/GCC check, QEMU archive size/hash checked before execution; build/emulation in network-disabled capability-dropped container with read-only inputs and NIC disabled, bounded emulator run. No artifact upload, real flash, OTA, production, security settings or protected branch changes. Exact QEMU archive bytes and SDK runtime compatibility are not yet verified by this session; workflow verifies archive before executing. Administrative settings/full current history audit remain incomplete; no claim of full audit. No private/paid CI.
- Run 45930476924 / job 140281684348 created, observed in_progress at capture. No runtime PASS or measurements asserted in this start entry. Next inspect final run, complete log, archive verification, compiler/build and UART runtime marker with all numeric metrics; correct issues only after evidence and audit. Emulator metrics do not establish physical ESP32-C3 latency, actual integrated adapter behavior, Wi-Fi/TLS heap or hardware safety. Preserve prior checkpoint; no merge of test PR.

---

# 2026-11-27 — Isolated ESP32-C3 completion model compile/link PASS

- Run 45928493648 / job 140275401665, workflow head 5321dac7747d551a5fdcd2d5e573c02223670931, workflow blob 5d3a5c28f3be1afa05564ddbaa0740aac0bd1fe7, model source e38a62bb146f56c90094bf939fe16f1eede7c14d. Run completed success; one job and all three steps successful. Full log retrieved (208512 characters, 1663 lines), 1017/1017 build steps, final ACK_ADAPTER_COMPILE_LINK_OK. ESP-IDF 6.1.1, GCC 15.2.0, elf32-littleriscv, linked probe/init/clock/atomic capability and esp_timer_get_time; disassembly confirms call to ack_step. All three pinned Git blob checks passed; logged SHA256 completion adapter 56cd103d214d35f457b9e8099898e15ffa10f3ec51a608e1f2acec63ba0ee14c, mailbox a992d1122351ff31ec7114cff33f3192b38bb81972f2b990195a7a1a6da533b2, worker 70567b0938766cf0317fa2ae96c2c92e255d12587a2e85bf35b76b521edcca1c. Test image 0x25740 = 153408 bytes, not full sensor firmware. No uploaded artifacts by design, no independent binary download or device execution.
- Five SDK CMake component-validation warnings about private include directories shared by esp_wifi and wpa_supplicant; Bluetooth Kconfig choice-symbol notes. No compiler warning/error markers in reviewed log. Metadata read only; no observed secret leakage in reviewed log, but administrative visibility and current full-history audit remain incomplete. ATOMIC_BOOL_LOCK_FREE returns 1 in disassembly (sometimes lock-free); do not infer nonblocking timing guarantee on target. Empty app_main means no runtime test. PR #30 closed without merge, branch and logs preserved. Previous Linux tests not repeated. No production firmware, OTA, physical device or protected branch changes.
- Next: qualify integrated runtime scheduling/resource lifetime on ESP32-C3, verify actual task timing and memory/stack usage, then assess remaining Wi-Fi setup gates (trust, storage and physical tests). Do not treat successful compile/link as hardware validation or authorize live activation. No paid/private CI. Preserve prior checkpoint.

---

# 2026-11-27 — Isolated ESP32-C3 completion model compile/link started

- Source checkpoint e38a62bb146f56c90094bf939fe16f1eede7c14d. Local environment checked: gcc and clang available; idf.py, riscv32-esp-elf-gcc, docker and podman absent. Existing Linux regression results retained; no repeated tests. Espressif ESP Timer documentation (https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-reference/system/esp_timer.html) reviewed for component and monotonic timer usage.
- Draft PR #30, branch ci/ack-adapter-probe-20261127, head 5321dac7747d551a5fdcd2d5e573c02223670931, workflow blob 5d3a5c28f3be1afa05564ddbaa0740aac0bd1fe7. One added workflow only, based on previously successful isolated PR #29, with exact three public model headers pinned by Git blobs: completion_adapter 47104a703fbae97821b0d163de594dc2a0eb8113, mailbox c73f0dd25a66edd063f30d314a40b590acebe859, worker bbb110d76bffede4942330ce7d59a99eca8af42d. Empty app_main, force-linked model entry/init/clock and atomic capability symbols, no runtime invocation. SDK 6.1.1 commit 051e4c4eb20346e043cdd47bc388fde75ff8af63 and GCC 15.2.0 required, image digest sha256:e5b767c64fdfdf427aa14cc7aaa15d24caf2f22c78241c5a3ed18150cfa7f05c. Same-repository draft PR only, permissions {}, standard public runner, 12 min cap, no external Actions, network-disabled capability-dropped build container, no upload or private inputs. Existing main workflow triggers reviewed, unchanged. No production or protected-branch merge.
- Run 45928493648 / job 140275401665 confirmed in_progress at capture; setup step success, compile/link step in_progress, zero artifacts. No PASS asserted. Next retrieve final state, all jobs/steps/full logs, confirm exact source checks, build and linked symbols, warnings and token rights, close test PR without merge after review. Local SDK unavailable; actual target scheduling, full firmware and hardware tests still pending. Administrative secret visibility and complete current-history audit remain incomplete; no claim of full audit. No device, OTA, paid/private CI or production changes. Preserve prior checkpoints, no force push.

---

# 2026-11-27 — Offline channel stop and expiry contention

- Base 8e929fab77f248351cf2091dedbaa636f9172859. Added standalone tests/browser_worker/test_channel_stop.c only plus three journals. All seven retained local input Git blobs independently matched the fixed public tree, including actual room_phone_channel.c, mock, adapter and prior threaded test. No repeat of previous successful exchange regression. Four actual two-pthread scenarios: stop or logical expiry while adapter lock is held by worker; each with timely or late exit/drain. Owner retains pending response until separate simulated buffer wipe and actual successful pthread_join; failed/expired exchanges do not report success. With late drain the model stays stopped and retains pending ownership, not a successful cleanup. Clock deliberately held inside test callback to force contention; logical time advanced by fixture, not physical 2-second measurements. No live network, SDK or credentials.
- Actual GCC 14.2.0 and Clang 17.0.0 strict C11 -Wall -Wextra -Werror -pedantic -pthread -g -O1 -fsanitize=address,undefined -fno-sanitize-recover=all -fno-pie -no-pie, compile test_channel_stop.c plus unchanged main/room_phone_channel.c with include paths to tests/browser_worker, tests/browser_worker/mock_channel and src; timeout 20 s; both exit 0, 4 scenarios and 56 checks each. GCC separate ThreadSanitizer executable linked libtsan.so.2, exit 0 twice, second capture 60 bytes, 4 scenarios and 56 checks PASS, no reported race. New exact Git blob 362ff6b4257c57e6ffdf8913fb1e85aac824bc16; SHA256 1f326f371d16ffa8dec5792e97f83903f0d740a6058101a92def343ccc440696. This is not proof of race absence, actual SDK stop latency or complete zeroization.
- Static review of actual task close (fa4d05c63ad571ec52b6b418c3eacbf6f6f02813) and session close (d799532be5623d402b4c241264c33cb4f849ad68): close requires valid owner, separate sender-drained proof and radio barrier, uses five-second logical cleanup budget, quarantines failed teardown. Integration must retain context after failed close, not reuse or free it; hardware/SDK recovery still unproven. No production source changed. Existing two-second worker budget and five-second session budget are separate model contracts; scheduling and combined shutdown must be verified before activation.
- No new dependencies, workflow, private source or real credentials in test. Candidate tree has no workflows. Historical full public history scan and subsequent limited review do not certify current entire history; admin secret visibility incomplete. No device, OTA, deployment, paid/private CI or protected branch changes. Local ESP-IDF, Docker and Espressif cross compiler absent. Next: isolated ESP32-C3 compile/link of adapter with pinned SDK and audit, then target scheduling and SDK resource lifetime, trust/storage/physical verification; no live activation. Revert isolated test commit if required.

---

# 2026-11-27 — Actual two-thread phone channel integration

- Resumed from verified public checkpoint 455261621642588d907da54fec6140b4a58b4e81, with prior 100-exchange serialized regression already synchronized. Added isolated tests/browser_worker/test_channel_threads.c only plus journals. Actual unchanged room_phone_channel.c (ce7369f90833bd796ed2dab0b11579c637f6b487), channel header ef0c5d44c5a32ac190b71fb38000da876d8ad618, mock be40b2441aa79b980c868fd79981d3971ed5bfd1, adapter 47104a703fbae97821b0d163de594dc2a0eb8113, mailbox c73f0dd25a66edd063f30d314a40b590acebe859 and worker bbb110d76bffede4942330ce7d59a99eca8af42d were matched to local Git blobs. Existing test blob also matched. No repeated earlier tests.
- Two real Linux pthread execution contexts; only main/owner accesses channel and task state, separate worker owns synthetic request/reply buffers, copied mailbox and receipt synchronized through existing adapter, CLOCK_MONOTONIC, atomics for fixture coordination. 1000 complete exchanges per run; owner observes WAIT before worker completion, exactly one success per exchange, no outstanding response at end; worker joined before mock drain. Test-only retry loops bounded by process timeout. No real I/O or SDK.
- Actual GCC 14.2.0 and Clang 17.0.0, strict C11 -Wall -Wextra -Werror -pedantic -pthread -g -O1 -fsanitize=address,undefined -fno-sanitize-recover=all -fno-pie -no-pie, compile test_channel_threads.c with unchanged room_phone_channel.c and mock/channel/adapter include paths, timeout 20 s: both exit 0. Separate GCC build -fsanitize=thread (other flags identical), exit 0, 1000 exchanges, no reported race. Tested and uploaded blob a2b3e1863f1a1daf4e7d5c11989018dd33dc5955; SHA256 53bccd4bdb1348d2872e90e904c44599f57de2c9cbe2610d5fc5a7ba3761c1bb. Not proof of absence of races or target scheduling.
- Previous branch advanced between turns and already has synchronized journals; preserve it, new isolated test branch from fixed checkpoint only. No new dependency, executable workflow, private source or real credential added. Direct Git download failed DNS; local retained source matched pinned GitHub tree. Administrative audit remains incomplete, earlier Gitleaks scan is historical; current new test reviewed as synthetic. No firmware change, production, OTA, paid or private CI. Next: actual stop/expiry contention with separate exit/drain evidence; review close failure and resource lifetime; qualify target compile/link then physical timing. No activation. Revert isolated test commit if required.

---

# 2026-11-27 — Multi-exchange offline phone channel regression

- Base a2bff1b9176b9b1d61c4bbaa4072cd40704fa785. Extended existing test_channel_ack.c only; actual channel source and mock unchanged. 100 sequential exchanges within one open session, increasing tickets and distinct response bytes, reject stale send and receipt events, reject duplicate receipt, wait before consumption and block next submit until receipt consumed. Stop under simulated busy lock blocks owner polling even if sender completes before stop reaches mailbox; owner poll failure precedes ready receipt. No actual network, firmware, dependency or workflow changes.
- Actual GCC 14.2.0 and Clang 17.0.0 strict C11 ASan/UBSan, same compile flags and command as previous entry, each timeout 10 s, exit 0, 1823 checks PASS per compiler (includes previous integration cases as necessary regression). Test blob 18f88d159269cbe942f1b98738d18ed0a766af92, SHA256 f96db429100dd8206ad7ddadc93108d7062c323094d93858530ef2c39678d928. Local actual channel and mock verified against previous Git blobs; no new CI. Diff command exit 1 denotes expected differences, not failed tests.
- Limitations: serialized fixture with simulated contention and owner poll failure, not a real task deadline implementation or hardware proof. Prior public history audit limited to recorded checkpoint plus heuristic review; admin secret visibility remains incomplete. No private data introduced, no new executable code outside tests, no change to protected branches, no deployment or OTA.
- Next: separate two-thread actual channel integration with owner-only channel access and synchronized mock transport state, test stop/expiry under real contention; review actual task close failure and resource lifetime before SDK integration. Revert isolated test commit if needed.

---

# 2026-11-27 — Offline actual phone channel integration

- Base 84f1c760bbf36df6e7703673c0b73eed16d40e1f. Rechecked existing local adapter and mailbox Git blobs and public journals, no repeat of prior CI. Direct raw download failed DNS (curl exit 6); reconstructed unchanged main/room_phone_channel.c and src/room_phone_channel.h from connector, verified exact blobs ce7369f90833bd796ed2dab0b11579c637f6b487 and ef0c5d44c5a32ac190b71fb38000da876d8ad618.
- New serialized host-only test compiles actual channel source, stubs APSTA task and uses existing completion adapter. Partial simulated output and copied bytes yield WAIT, overrun rejected, full completion yields one successful acknowledgement, BUSY yields WAIT, partial failure, expiry, late receipt and stop fail closed; borrowed owner buffer released on unsuccessful send only after separate simulated exit and drain. Fixture is one exchange per window and not a real SDK or production transport implementation.
- Actual GCC 14.2.0 and Clang 17.0.0 strict C11 ASan/UBSan, -fno-sanitize-recover=all -fno-pie -no-pie -g -O1, timeout 10 s, both exit 0, 80 checks each. Reproduce at repo root: cc -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-sanitize-recover=all -fno-pie -no-pie -g -O1 -include tests/browser_worker/mock_channel/room_apsta_task.h -Isrc -Itests/browser_worker main/room_phone_channel.c tests/browser_worker/test_channel_ack.c -o /tmp/channel-test && timeout 10 /tmp/channel-test. Test blob 346a73a405097cad5429f9857cb66cf9e10dce2c, mock be40b2441aa79b980c868fd79981d3971ed5bfd1; SHA256 0fecf8c2aa0a5ebb1a00c9d5df6ea2f837ac2670acc6192087cbd79b913bed3b and 219163989e63668fd32a3a05b5cd47358fe8f66d55de0a30512587fd80ad8fa0.
- No code outside tests, workflow, dependency, real network, device or heating changes. Next: multiple exchanges per window and explicit bounded stop/receipt handling under contention with mock task deadlines; actual ESP32-C3 scheduling, secure browser transport and hardware verification remain required. Revert isolated test commit if needed. No claim full security audit.

---

# 2026-11-27 — Actual two-thread completion adapter stress

- On adapter checkpoint d7f8f1a4e3cdc6aa2333622fb0de20c0a23d9195 added tests/browser_worker/test_ack_threads.c only. Two real Linux pthreads, CLOCK_MONOTONIC sampled inside adapter lock; distinct thread-local buffers, 10000 correlated copied request/reply round trips per run. Each owner observes WAIT before worker signals completion, then consumes exactly one matching receipt and rejects repeated polls. Stop and separate drain proof checked. Bounded retry/yield belongs only to test harness, not owner implementation.
- Actual GCC 14.2.0 and Clang 17.0.0 strict C11, -pthread -fsanitize=address,undefined -fno-sanitize-recover=all -fno-pie -no-pie -g -O1, timeout 20 s: both exit 0, 10000 round trips each. Separate GCC -fsanitize=thread executable linked libtsan.so.2; two invocations exited 0 (second captured 55-byte PASS output, no reported races), not proof of race absence. Exact test blob ffd66dd5f4f178a895baf2f6f49c413329b415ff, SHA256 4ebc2cb3bea51f5422aca2d1ac0436cbf25e01430ade4938a2ed3b900a008840; unchanged adapter blob 47104a703fbae97821b0d163de594dc2a0eb8113 and prior mailbox/worker blobs verified. No real SDK, network, new dependencies, firmware, workflow or protected branch changes.
- Next: integrate with bounded room_phone_channel mock and verify partial-send/expiry/stop result mapping without treating copied bytes as sent. Actual ESP32-C3 task latency, hardware and browser trust remain open; no live activation. No duplicate historical builds or paid/private CI. Revert isolated test commit if required.

---

# 2026-11-27 — Offline synchronized completion receipt adapter

- Base 805312e2e24e5348da712bd9e304842fa14c0775. PR #29 already closed without merge; run 45920972097 and job 140251360914 independently confirmed success, all three steps and 1017/1017 build steps; full retrieved log 208984 characters. No repeated CI or previous completion regression.
- Added isolated tests/browser_worker/completion_adapter.h and test_ack_adapter.c. A single outer try-lock serializes the existing copied mailbox and one correlated receipt. Owner polls by window and ticket; copy is not send completion; next submission waits until receipt consumed; failures, stop, expiry and clock faults invalidate receipt; actual worker exit and buffer drain remain separate. Trusted local events only, no real I/O, no firmware integration, no new dependencies.
- Actual local GCC 14.2.0 and Clang 17.0.0 strict C11 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-sanitize-recover=all -fno-pie -no-pie -g -O1 test_ack_adapter.c; timeout 10 s, exit 0 on each, 88 checks per compiler. Existing mailbox and worker headers reconstructed from connector after raw GitHub DNS failure (curl exit 6); exact Git blobs c73f0dd25a66edd063f30d314a40b590acebe859 and bbb110d76bffede4942330ce7d59a99eca8af42d matched. New tested blobs 47104a703fbae97821b0d163de594dc2a0eb8113 and 10ec3812a774cbafb9c761adf8edf44a51a98607; SHA256 56cd103d214d35f457b9e8099898e15ffa10f3ec51a608e1f2acec63ba0ee14c and 0f873bbf7b94b519ffa0f6ec7897e8b722879bf9203ed21504073bc5828de649.
- Public history scan run 45903480435 verified 280 commits, 60 refs, zero findings as of candidate 3c73e6795f8839997256c5914632a6a60b20820c. Checked added lines in all 17 subsequent commit patches for common credential formats and private repository names: no matches; limited heuristic, not a new full Gitleaks scan. Candidate tree has no workflows or submodules; main workflows manually triggered or main-path restricted, permissions {}. Administrative secret settings remain not independently readable; no secrets, executable workflows, new dependencies or artifact upload introduced. No paid/private CI, protected merge, device, OTA or production changes.
- Next: actual two-thread scheduling test of adapter and bounded owner-channel mock integration; verify no direct mailbox access and no lock wait/spin in owner. Current tests are serialized with simulated contention, not a race-detector run of this adapter or device scheduling evidence. Retain all HTTPS trust, physical DHCP/heap/shutdown, storage and hardware gates. Revert isolated commit if needed; preserve baseline.

---

# 2026-11-27 — Offline completion contract integration regression

- Reviewed existing copied mailbox and owner channel contracts: taking a copied reply is not sending it; a second TAKE_REPLY is rejected both before and after successful send completion, so this observation cannot serve as an acknowledgement. The real adapter needs an explicit, correlated completion indication to the owner and must not report successful send on copy alone. Failure or expiry must not produce a successful receipt. External buffer drain and worker exit remain independent proofs.
- Added standalone tests/browser_worker/test_completion.c, a serialized synthetic sender fixture only, no live transport or firmware integration. Actual local GCC 14.2.0 and Clang 17.0.0 strict C11, ASan/UBSan, -fno-sanitize-recover=all -fno-pie -no-pie, both exit 0 under 10 s timeout, 45 checks each. Exact local baseline blobs mailbox c73f0dd25a66edd063f30d314a40b590acebe859 and worker bbb110d76bffede4942330ce7d59a99eca8af42d matched GitHub. Tested new blob b8049ea6fac862deffe2466e71b7f5765d377594, SHA256 b95954e891d5d3075cb88236fa98a90ecf7bda772424ac982880973c2fe11b04. Covers early, repeated, mismatched, failed and late completion, stop and drain. Synthetic receipt is not a synchronized production implementation or proof of actual send.
- Next: design explicit owner/worker completion result and lifetime contract, then integrate a bounded mock adapter and test scheduling; real worker shutdown, physical target timing and browser trust remain open. No network, firmware, dependencies, workflow, device or OTA changes. Revert isolated test commit if needed.

---

# 2026-11-27 — ESP32-C3 clock/synchronization compile-link PASS

- PR #29, head ccf4f2ec264f7467ded1b5b44332f008fafc4b9b, workflow fc838f094208be373969811b0d7a779447ab1d72, run 45920972097 / job 140251360914: success, all three steps. Full log reviewed: 1017/1017, real ELF and binary, CLOCK_SYNC_COMPILE_LINK_OK; SDK 6.1.1, GCC 15.2.0, exact pinned mailbox and worker headers verified by successful workflow hash checks. ELF elf32-littleriscv includes room_sync_probe, room_sync_init, room_sync_clock, room_sync_atomic_capability and esp_timer_get_time. Probe binary 0x25360 bytes; standalone default layout, NOT the candidate partition or firmware size.
- Disassembly of room_sync_atomic_capability returns 1 (sometimes lock-free), not 2; no guarantee of lock-free operation or bounded scheduling on the device. SDK Kconfig deprecation/choice notices present; no compiler warning/error markers found in reviewed log. Token Metadata: read, zero artifacts. No actual probe execution, Wi-Fi, flash, OTA or protected-branch merge. Administrative security visibility remains limited.
- Compile/link gate satisfied for the isolated model only. Close PR #29 without merge, preserve source and run. Next: offline copied-buffer send completion contract and mock integration tests; actual ESP32-C3 task scheduling, timer latency, TLS trust and resource quiescence remain unverified. Do not activate networking or alter production configuration. Prior pending entry below is historical.

---

# 2026-11-27 — ESP32-C3 clock and synchronization compatibility probe

- Source review: ESP-IDF 6.1.1 commit 051e4c4eb20346e043cdd47bc388fde75ff8af63, esp_timer.h blob e48a213634d3e2cd6ba1dec657687a756d48c291, stdatomic.c 428a800eca518e00b17ead979b2c7acef5aa9633, esp_stdatomic.h a988075a6ffd3022b6be10e5c0d038467ec2b41c. SDK provides software atomic operations on single-core targets without native A extension using interrupt masking. esp_timer_get_time returns signed 64-bit microseconds; documented fast nonlocking reads, deep-sleep resets clock; discard session across restart. https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-reference/system/esp_timer.html
- Local generic Clang 17 RV32IMC freestanding assembly-only probe exit 0, two atomic-alignment warnings, emitted __atomic_compare_exchange_1 and __atomic_store_1 calls, ATOMIC_BOOL_LOCK_FREE macro 1; not Espressif GCC, not linked or run. Local SDK/compiler/container runtime absent. No lock-free or target latency claim. Keep task-only ownership; no spin retry at elevated priority.
- Isolated draft PR #29, ci/clock-sync-probe-20261127, commit ccf4f2ec264f7467ded1b5b44332f008fafc4b9b, workflow blob fc838f094208be373969811b0d7a779447ab1d72; run 45920972097 in_progress at capture, no result yet. Exact two public headers pinned by SHA, standalone empty app_main, offline capability-dropped image sha256:e5b767c64fdfdf427aa14cc7aaa15d24caf2f22c78241c5a3ed18150cfa7f05c, GCC 15.2.0, permissions {}, standard public runner, 12 min limit. Only workflow added on separate branch, no merge, no secrets referenced, no artifact upload or device writes. Administrative secret/settings visibility still limited; no claim full audit.
- Next: inspect run status, all jobs, full logs and linked symbols before accepting compile/link compatibility; then test actual task scheduling/latency, synchronization and timer on hardware separately. No firmware change, real SDK worker or hardware activation. Do not duplicate run. Close test PR without merge after result review.

---

# 2026-11-27 — Offline mailbox clock ordering and race-detector test

- Host-only mailbox now samples one trusted monotonic clock AFTER acquiring the atomic try-lock; event-supplied time is ignored in the clocked entry. Busy does not sample clock. Clock failure stops/faults the model and wipes its slot; backwards time still faults. Legacy explicit-time entry retained for offline regression. Shared real-time callers must use the same bounded non-reentrant clock; this remains a caller contract, not a live adapter.
- Actual local Linux GCC 14.2.0 and Clang 17.0.0 strict C11 -Wall -Wextra -Werror -pedantic -g -O1 -fsanitize=address,undefined -fno-sanitize-recover=all -pthread: both old test_mailbox (35 checks + 10000 fixed-clock round trips) and new test_clock (20 checks + 10000 CLOCK_MONOTONIC two-thread round trips) exit 0 under 20 s timeout. GCC separate -fsanitize=thread run of test_clock exit 0, 20 checks + 10000 real-clock round trips, no reported races. This is not proof of absence of races. Host atomic_bool lock-free test passed. New tested blobs mailbox c73f0dd25a66edd063f30d314a40b590acebe859, test 80b666ce1eee51a944c54be7de4e4707d15048fe. SHA256 a992d1122351ff31ec7114cff33f3192b38bb81972f2b990195a7a1a6da533b2 and b7196eca06536855871bd163cf28f8b995aab640e449fdb891c4e26c3a2b31df. Baseline worker and test_mailbox unchanged, local baseline blobs matched GitHub.
- Espressif stable ESP32-C3 timer documentation (https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-reference/system/esp_timer.html) supports considering esp_timer_get_time for the target clock; exact target atomic implementation and timing, real SDK worker exit/drain and target build not yet verified. No live networking, dependencies, firmware, production configuration, CI, flash, OTA or protected merge. Next: isolated target compile probe for synchronization and timer, and mock room_phone_io adapter with actual send acknowledgement; retain trust/Security2/hardware gates. Revert this isolated change if required.

---

# 2026-11-27 — Offline copied mailbox and threaded stress test

- Added tests/browser_worker/mailbox.h and test_mailbox.c, host-only and not linked into firmware. Single 4096-byte copied slot, C11 atomic try-lock, explicit busy/reject/success, one in-flight exchange, window/ticket checks, copy before send acknowledgement, volatile clearing of internal slot on consumption/stop. No borrowed pointers, network, credentials, SDK calls or external dependencies. Existing worker_model.h unchanged (bbb110d76bffede4942330ce7d59a99eca8af42d).
- Actual isolated Linux GCC 14.2.0 and Clang 17.0.0 strict C11 -Wall -Wextra -Werror -pedantic -g -O1 -fsanitize=address,undefined -fno-sanitize-recover=all -pthread; both exit 0 under 20 s timeout, each 35 checks + 10000 two-thread round trips PASS. Tested Git blobs mailbox 90222d5334a5beb50ab058f2b767f7e5c70f1e69, test 7c02e0cdf462c5b8084c9b896e779c246912ec99; SHA256 b2857dbe8ffc93ce1d7c5c8d6c0f908bb9c368ccc63debb09c1264c25d768e60 and b60fa8f4d54b25f72f3561d4f2b767a8652431c232d203c8de386ca947593421. Reproduce from tests/browser_worker with the same flags and test_mailbox.c. Synthetic packets only.
- Limitations: ASan/UBSan not a race detector; stress uses fixed timestamp, timeout tests serialized. Real-time ordering across tasks, lock-free atomic implementation on ESP32-C3, external buffer lifetime/clearing and actual SDK worker exit/drain not verified. Current model requires trusted ordered timestamps and disjoint private caller buffers. No blocking SDK stop in owner. Next: resolve clock ordering and target synchronization, then room_phone_io mock adapter and send-completion semantics, integrated tests and target build. TLS trust/Security2, hardware isolation/DHCP/storage and physical rollback remain open. No firmware, workflow, protected branch, flash or OTA changes. Revert isolated commit to remove model.

---

# 2026-11-27 — Offline server worker lifecycle model

- Standalone C11 model and tests under tests/browser_worker, not linked or activated in firmware. Serialized trusted local events only; no sockets, HTTP parser, TLS, credential handling, actual buffer copying, synchronization or SDK calls. Model enforces one in-flight request, 1..4096-byte lengths, window/ticket matching, 2 s start/exchange deadline without extension on reply, 2 s stop acknowledgement budget after observed stop request, and no reuse before actual worker exit AND callback/buffer drain. Timeout during stop permanently locks model for this boot. It does not make a blocking SDK function bounded or establish real resource quiescence. Actual owner must poll and serialize events; Wi-Fi teardown/handoff blocked until proved safe.
- Official ESP-IDF HTTP server docs confirm blocking stop and interface binding: https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-reference/protocols/esp_http_server.html ; HTTPS SDK audit retained. No blocking SDK stop in Wi-Fi owner.
- Actual isolated Linux tests GCC 14.2.0 and Clang 17.0.0, strict C11 -Wall -Wextra -Werror -pedantic ASan/UBSan nonrecovering, each exit 0 under 10 s timeout; 62 assertions each. Tested files Git blobs bbb110d76bffede4942330ce7d59a99eca8af42d and 7efa79326235be59c061979b16d9713b6e03dd77. Re-run from tests/browser_worker: gcc -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=undefined,address -fno-sanitize-recover=all -g -O1 test_worker_model.c -o /tmp/worker-test && timeout 10 /tmp/worker-test (or clang).
- No actual worker, copied mailbox or threading yet; next design bounded copied mailbox with ownership and synchronization, integrate with room_phone_io and SDK worker mock, verify late completion and shutdown, then target build against committed partition. Browser certificate trust/Security2, DHCP, TLS heap, SDK shutdown and hardware remain release blockers. No dependencies, production config, CI, flash, OTA or protected merge. Revert this isolated commit if needed.

---

# 2026-11-27 — Independent Node test of published browser model

- Source commit 68f2d938aeab117ba5ce0fa59df648069557518b. Direct raw GitHub download failed (sandbox DNS); fetched exact files through GitHub connector, reconstructed local copies and verified git hash-object against both published blobs: model af0d7cfae55ca939b320f0d96774dac8a6508440, test 162464cf43c78b72afd4d5d374566c6e056a6afa. SHA256 model 388502b5ef3bcda41509b22faeffa096ecebc06fdc1340d46a05de67d911403d, test c78f015239bbebf65d567e7075214797e5721202509c18f47fdb9e5f0f0589b4.
- Local Node.js v22.16.0: node --check on both modules exit 0; node test_view_model.mjs exit 0, 114 assertions PASS. Prior direct V8 run also 114 assertions PASS. No external packages, network requests, credential collection, CI, or firmware changes. Actual mobile browser, HTTPS and SDK end-to-end interoperability not tested.
- Next: isolated bounded server worker contract and browser trust/Security2 interoperability review, then integrated tests and target build with committed partition; preserve fail-closed behavior and no live credential entry or device writes before hardware gates.

---

# 2026-11-27 — Offline browser presentation state model

- Added isolated ES module and tests under tests/browser_setup (not included in firmware or served by preview). No network, DOM, credential collection, cryptography, dependencies, storage, workflows, or startup changes. Based on exact src/room_setup_dispatch.h and room_setup_request.h contracts and existing docs/BROWSER_SETUP.md. The view is NOT an authentication or authorization implementation.
- Actual V8 execution before publication: 114 assertions PASS, including full reply code/phase/token-presence matrix, exact 64-bit token handling, one outstanding request, matching local reply ticket, matching trial, explicit save confirmation, fresh verified status, terminal receipts, ambiguous/lost/late save without retry, cancellation, expiry and monotonic time. Test source generated from the same executed function definitions. Browser/Node and SDK interoperability not yet run.
- Independent trust anchor, actual Security 2 browser client, AP-only SDK worker with bounded shutdown, real DHCP freshness and protected storage, runtime heap, hardware and target compilation remain open. No actual password entry or live endpoint. No flash/OTA/protected merge. Next: run standalone Node tests on published files, then review secure browser client and implement isolated bounded backend worker model; do not enable real input or listener until all trust and lifecycle gates pass. Revert this commit to remove isolated model if needed.

---

# 2026-11-27 — Approved 2 MiB layout adopted in experimental candidate

- User approved candidate adoption only. Base cd786d0de71d3824c8d84ea1fedda42d925d6a56 differs from the previously tested source 75df361f733973bd7d58e7bd856af96da28bdbb6 only in three journals (GitHub compare). Apply exact CSV transformation from successful PR #28 / run 45912021281: move factory to 0x120000 / 2 MiB, sort after room_wifi; no CMake, sdkconfig, runtime or workflow edits. Local arithmetic/structural checks passed: row order, all data entries identical, app alignment and size, nonoverlap and 4 MiB bounds, original factory range excluded.
- Prior SDK-generated table and inert HTTPS build evidence retained; no fresh target compilation for this committed candidate yet. Prior experiment applied temporary HTTPS linkage that is NOT adopted here. No duplicate run for the identical CSV alone; next target build must verify actual candidate table hash b7e7cc4ebfcb5ce7d928eb74f9aa45866890d09399e7b9c44f3b0fa38cb37246 and generated application offset. No device writes, OTA, protected merge or production release. Roll back source change by reviewed revert; hardware recovery not proven. Next: trusted browser interface design and validation, then isolated target build with actual candidate. Existing DHCP freshness, bounded shutdown, certificate trust, Security2 interoperability and physical flash/heap gates remain.

---

# 2026-11-27 — Offline 2 MiB partition experiment PASS

- PR #28 head 3c6b04341752471b3d27a5468264d8e3ff5957e9, workflow blob 9535132ee46b53d95af3487f37a2341ea0cab4f9, run 45912021281 / job 140222386233 completed success, all three steps. Full log 240058 characters / 1842 lines; actual 1029/1029 build, BUILD_EXIT=0; SDK 6.1.1, GCC 15.2.0, source 75df361f733973bd7d58e7bd856af96da28bdbb6. Log lists 48 source blobs; four critical build/config inputs independently matched; full per-blob recheck hit connector call limit. Zero warning/error markers or common credential signatures detected in log; not full security clearance. Token Metadata read only; no artifacts.
- CSV_ORDER_OK, DATA_PARTITIONS_UNCHANGED, PARTITION_GENERATOR_OK, APP_OFFSET_OK=0x120000, GENERATED_TABLE_MATCH_OK, HTTPS_PARTITION_2M_BUILD_OK, PARTITION_EXPERIMENT_OK. Generated table SHA256 b7e7cc4ebfcb5ce7d928eb74f9aa45866890d09399e7b9c44f3b0fa38cb37246 equals independently generated table; old table ea697e36954d8d3fe4b112e9693ab32764e90bf81507434711b83eff4f19c20f. All eight forced HTTPS symbols and Security2 checked by workflow. App 1053392 bytes (+119168); headroom 1043760 of 2097152; static DRAM 112930/321296, remaining static 208366, not runtime heap.
- Offline layout qualifies for continued design only. No actual partition/config changes, runtime listener, device flash, OTA, deployment or main merge; physical 4 MiB capacity, power-loss/rollback and TLS runtime heap remain unverified. Do not rerun this experiment without new reason. Preserve old partition and app; next: separately approved candidate partition adoption if needed, trusted browser backend design (certificate trust, exclusive AP/STA ownership, bounded shutdown, DHCP freshness and Security2 interoperability), then mock and target tests; no hardware activation. Existing protected branch and 5.5.6 baseline retained.

---

# 2026-11-27 — Partition CSV ordering correction

- PR #28 initial run 45910641021, job 140217880963, workflow b66009136bb4481e69438fa11179eda85e996b41 failed before compilation: SDK CSV generator reported line 6 overlap because relocated factory row preceded the lower-offset room_wifi row. Addresses themselves do not intersect. No firmware was compiled or flashed by that attempt.
- Corrected only experimental workflow in commit 3c6b04341752471b3d27a5468264d8e3ff5957e9: remove original factory row, append relocated factory after room_wifi, assert exact order, offsets and 2M size before SDK validation. No security check removed. All original data offsets and proposed factory range unchanged.
- Replacement run 45912021281 status at capture in_progress, conclusion pending. Do not duplicate. Review final generator/roundtrip/flash manifest and build results before accepting layout. No candidate code/config change, main merge, firmware upload, hardware operation or OTA. Physical Flash and rollback remain untested.

---

# 2026-11-27 — Approved offline 2 MiB partition validation

- User explicitly approved experiment only: relocated 2 MiB factory at 0x120000, preserve NVS/PHY/room_wifi offsets, no device writes or OTA. Draft PR #28, ci/partition-2m-20261127, workflow b66009136bb4481e69438fa11179eda85e996b41; run 45910641021, status in_progress, conclusion pending. Do not duplicate. Inspect actual SDK generation, build, symbols, size, flash manifest and all checks before claiming PASS.
- Pinned public source 75df361f733973bd7d58e7bd856af96da28bdbb6; firmware inputs identical to HTTPS footprint experiment. Temporary container CSV and CMake changes only, not applied to candidate or main. Same qualified SDK/image/GCC, offline container and minimal token. SDK generator validates 4 MiB old/new tables and binary roundtrip, data partition equality, built partition binary equality and expected app offset in generated flash manifest. Inert HTTPS linked, normal size checks retained, no binary upload/flash.
- Candidate app range 0x120000..0x31ffff, old factory area untouched in design. Expected free app space about 1043760 bytes, to be measured. Hardware flash capacity, boot and rollback behavior remain unverified. No production layout or OTA authorization. After result, review logs and decide whether to incorporate validated experimental layout into a separate source branch.

---

# 2026-11-27 — HTTPS footprint measured; partition does not fit

- PR #27 workflow c9fa2bad511cae2de85e1b8a67655f7b80510cc8, run 45906704171, job 140205056464 succeeded as a measurement, NOT a deployable build. Actual SDK build exit 2, explicit factory overflow 0x12d0, HTTPS_PARTITION_OVERFLOW_CONFIRMED and HTTPS_FOOTPRINT_MEASURED. No warning lines, token Metadata: read, zero uploaded artifacts. Pinned source 1d005d634705baa187af55fb21f7b548c642da93; no runtime HTTPS calls, partition change or flash.
- Linked HTTPS dependency measurement: bin 1053392 bytes vs baseline 934224 (+119168), current 1048576-byte factory partition exceeded by 4816. Static DRAM 112930 vs 112812 (+118); remaining static budget 208366, not runtime heap. Flash code 817006; total image size 1053024 unpadded. Actual TLS connections, certificates, browser UI, MQTT and runtime heap are not measured.
- Proposed design for review only: 2 MiB single factory app at 0x120000..0x31ffff, retain nvs 0x9000..0xefff, phy_init 0xf000..0xffff and room_wifi 0x110000..0x115fff unchanged; original factory region 0x10000..0x10ffff remains unused and intact if migration writes only new app/table, enabling a potential rollback to old table. 896 KiB free after new app. New app headroom against inert measurement 1043760 bytes. App offset 64 KiB aligned, all ranges inside declared 4 MiB. This is arithmetic/design review, not SDK-generated or hardware-verified layout. Do not modify actual partition table before explicit approval, physical flash verification, backup and recovery assessment; migration/rollback not proven, no OTA introduced. Reference: https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-guides/partition-tables.html
- Alternative: reduce footprint without weakening TLS/security, but current 1 MiB already exhausted before browser UI and MQTT. Recommend offline validation of proposed layout after approval; preserve previous known-good firmware and data, no device writes. Close standalone PR #27 without merge.

---

# 2026-11-27 — Inert HTTPS footprint experiment

- Draft PR #27, ci/https-footprint-20261127, workflow c9fa2bad511cae2de85e1b8a67655f7b80510cc8; run 45906704171 status at capture in_progress, conclusion pending. Do not duplicate; inspect final logs, linker symbols, size and build exit before reporting measurement.
- Pinned source 1d005d634705baa187af55fb21f7b548c642da93, all firmware input blob SHAs match qualified 2eb6ddfb4d281af79fe13a794a6f668cb6d97bf2. SDK 6.1.1 / GCC 15.2.0, same pinned public image. Temporary CMake copy only: force inclusion of HTTPS start/stop and representative request/response functions; no runtime calls, credentials, listener, OTA, partition edits or firmware upload. Offline capability-dropped container, read-only source, permissions {}.
- Current declared flash 4 MiB; factory offset 0x10000 size 1 MiB; room_wifi NVS offset 0x110000 size 0x6000. Baseline 934224 bytes and 114352 free. If measured binary exceeds partition, record explicit SDK partition error and negative headroom; do not bypass partition validation. Probe is dependency footprint, not complete browser backend or runtime TLS heap. No actual hardware flash capacity verified. Preserve prior build and protected branch. Any partition redesign requires separate approval and migration/rollback assessment.

---

# 2026-11-27 — ESP-IDF 6.1.1 qualification PASS

- PR #26 workflow d6c19f1fc4593af5ad4964a250c7f7b9338bd17f, run 45904358973, job 140197424290 completed success. Exact source 2eb6ddfb4d281af79fe13a794a6f668cb6d97bf2, SDK 051e4c4eb20346e043cdd47bc388fde75ff8af63, image sha256:e5b767c64fdfdf427aa14cc7aaa15d24caf2f22c78241c5a3ed18150cfa7f05c, RISC-V GCC 15.2.0. HTTPS_SDK_PROBE_OK, ESP32C3_BUILD_OK, all 51 GCC ASan/UBSan mock scenarios passed; Clang unavailable; no warning lines in job log. Token Metadata: read.
- Firmware bin 934224 bytes, 114352 bytes free in 1 MiB app partition (10.9%); +32240 bytes over 5.5.6 baseline 901984. Static DRAM 112812 vs 122030 bytes (-9218); remaining static budget 208484 vs 199266, not measured runtime heap. Compile-only HTTPS probe bin 150016 bytes; does not establish actual HTTPS server footprint or runtime behavior.
- Artifact 15157661167, 5036778 bytes, independently downloaded: ZIP SHA-256 1d1f9f7e9abb961ea5ef918c35a377732a660cf28567e0321c799a9df3fa5153 matches GitHub. All 21 entries CRC-valid; 20 SHA256SUMS and 5 binary checksums match; provenance and 51 PASS markers verified; no common credential signature hits in archive. Security2 only, HMAC key ID -1, no automatic security provisioning config. Retention 14 days. No flash/OTA.
- SDK 6.1.1 qualifies for subsequent isolated development, not production activation. Preserve 5.5.6 baseline. Close single-purpose PR #26 without merge; next stage: quantify actual HTTPS component footprint with an inert link probe and assess partition/RAM headroom, then design trusted local browser backend and tests. Real AP-only isolation, bounded worker shutdown, certificate provisioning, Security2 browser interoperability, hardware DHCP/NVS, MQTT remain unverified. No partition change, production configuration change or protected merge authorized by this qualification.

---

# 2026-11-27 — ESP-IDF 6.1.1 qualification in progress

- Draft PR #26, ci/idf611-qualification-20261127, workflow d6c19f1fc4593af5ad4964a250c7f7b9338bd17f, run 45904358973 in progress at capture. Do not duplicate. Read final job logs and independently verify artifact before claiming success.
- Source 2eb6ddfb4d281af79fe13a794a6f668cb6d97bf2, only four tests/https_sdk_probe files differ from compiled input 4616624dd00abe44f0b2dabce2b44b657c85383d; firmware unchanged. SDK 051e4c4eb20346e043cdd47bc388fde75ff8af63, recommended GCC 15.2.0, image sha256:e5b767c64fdfdf427aa14cc7aaa15d24caf2f22c78241c5a3ed18150cfa7f05c.
- Offline capability-dropped container, read-only public inputs, minimal token, no secrets/OTA/activation. Compile HTTPS probe and firmware, check config and symbols, run 51 host cases, record sizes and checksums. Baseline 5.5.6 and protected branch preserved. Hardware/browser tests pending.

---

# 2026-11-27 — Public Git history audit completed

- User supplied repository Actions settings screenshot: full SHA pin required, all external contributors require approval, default token read-only, Actions PR creation/approval disabled, all Actions allowed. User reports Actions secrets/variables and environments empty. Screenshot repository identity relies on user context; organization-wide settings not independently accessible. Retain full SHA requirement; no settings changed.
- Standalone draft PR #25, commit 25df1d21451966819e80289ea6344feddbcb1b13, run 45903480435, job 140194581236: Gitleaks 8.30.1 official archive SHA-256 verified, full mirror non-shallow, git fsck passed, 60 advertised refs, 280 reachable commits, refs inventory SHA-256 12b69a5865887ed34adbfc504605b7e7007487c319f1306857161ec8350dd64c; scanner exit 0, findings 0, HISTORY_SCAN_OK. Job token Metadata: read. No report published. Scope is reachable public history at scan time; not deleted/unadvertised refs, every possible secret format, or private repos. Candidate scanned at 3c73e6795f8839997256c5914632a6a60b20820c; protected 0043c1e9498f22228db950144c0f6665ec83cf23.
- Next: qualify agreed ESP-IDF 6.1.1 in separate single-purpose isolated CI; retain prior firmware source 4616624dd00abe44f0b2dabce2b44b657c85383d and SDK 5.5.6 baseline. Only public synthetic tests, no secrets, deployment, activation or protected merge. Verify pinned image and SDK source, actual probe compilation, target and host tests, memory footprint. Hardware/browser testing and administrative visibility limitations remain.

---

# 2026-11-27 — Reconciliation after experimental development audit

- PR #23 source checkpoint b9f7483691d96a36a6ffbe77ce5c7d713b6600bd; firmware source 4616624dd00abe44f0b2dabce2b44b657c85383d. Protected PR #17 remains 0043c1e9498f22228db950144c0f6665ec83cf23. PR #20 CI checkpoint 50256e555a09faba60d5524d523779ab006bfe99 closed without merge; all history retained. No firmware change.
- Read-only audit reviewed patches of all 14 source and 15 CI commits for common credential signatures and private project references; no hits. This is not a complete full-history secret scan. Latest jobs had Metadata: read only; repository/organization/environment secret settings not accessible, so publication audit is incomplete.
- Run 45787225969, job 139796962990, SDK 5.5.6: 51 GCC ASan/UBSan scenarios, target build and artifact checks passed; Clang unavailable. Binary 901984 bytes, 146592 bytes free in 1 MiB app partition. Artifact 15101191273 independently downloaded and checked locally: ZIP SHA-256 b8483d1b4928720355e94bf0bc1185814d4ee817f18bd2153950956ea87bb972 matches GitHub; all 20 entries CRC-valid; 19 SHA256SUMS entries and 5 binary checksums match; provenance source 4616624d; 51 PASS markers, no compiler warning lines, no common credential-pattern hits in archive. No real hardware or browser test.
- Run 45787226079, job 139796963520: metadata only, AMD64 manifest sha256:e5b767c64fdfdf427aa14cc7aaa15d24caf2f22c78241c5a3ed18150cfa7f05c; raw index SHA-256 2b4f966c37eaedc19542b0637a5f850b2eff9478d8e858192aa0073d92c2dc91. ESP-IDF 6.1.1 source 051e4c4eb20346e043cdd47bc388fde75ff8af63. Migration agreed, but image not executed and new SDK/probe not compiled. Earlier pending-digest statements superseded.
- Cumulative PR path filters caused a redundant baseline build when metadata workflow was added (157 seconds run elapsed). Historical PR #20 closed without deleting branch or altering workflows. Future registry-only and SDK-qualification work must use separate single-purpose branches/PRs from main with branch guards and paths; no repeated baseline run on documentation or registry-only change. No migration build until outstanding publication audit gates are resolved or explicitly scoped to safe isolated testing.
- Before further code publication: complete available secret/history/permissions checks, verify source manifest and external dependencies, keep protected branch untouched; actual SDK migration, RAM/stack, AP-only HTTPS, browser trust and Security2, DHCP/NVS hardware, MQTT remain open. No production activation or OTA.

---

# 2026-11-26 — Exact SDK HTTPS isolation audit

- Verified pinned ESP-IDF commit 880a203ae603d08a94d06aa01e0d6761958e5e9e reports v5.5.6. Its HTTP server binds wildcard address and lacks the interface-name configuration field. HTTPS uses the same HTTP server; stop waits for the server thread without an overall bounded deadline. SDK copies private key buffer and directly frees that particular copy; application wiping alone cannot certify full SDK key erasure.
- Verified official v6.1.1 tag resolves to 051e4c4eb20346e043cdd47bc388fde75ff8af63. Header and implementation expose interface-specific bind with failure returned, and HTTPS default configuration includes it. Stop remains blocking. Prefer isolated migration qualification over custom HTTP parsing or wildcard listener. Exact v6.1.1 Docker digest not verified; no toolchain change, new CI run or network activation. Added standalone compile-only HTTPS API probe (not yet built) and source evidence in docs/HTTPS_SDK_AUDIT.md. No real keys, eFuse, protected-branch merge or deployment.

---

# 2026-11-26 — Browser trust gate and inert Russian preview

- Verified run 45784779902: 51 GCC ASan/UBSan scenarios and target build for 4616624dd00abe44f0b2dabce2b44b657c85383d; artifact 15099603667; CI digest f8b62b8d3900284c5600f23adebdd965fb0517679c23a04acb7199ea33416beb (independent archive verification pending). Clang absent.
- Added docs/BROWSER_SETUP.md and docs/setup-preview.html. Ordinary private-IP HTTP is not a trusted browser origin for Web Crypto; choose authenticated HTTPS as proposed design pending actual phone trust setup and identity provisioning review. No plaintext credential path. Preview is disabled, no JS/network/form submission, no firmware embedding. Checked static markup for absence of active or external resources; not rendered on phone.
- Documentation/UI-only commit, no firmware source or CI changes; no new firmware build necessary to validate documentation. Existing successful build belongs to earlier firmware commit. Remaining: review exact SDK HTTPS APIs, actual trust provisioning and browser Security2 interoperability, implement bounded AP-only backend and test hardware. No activation, eFuse, deployment, protected merge or Cloudflare changes.

---

# 2026-11-26 — Optional bounded phone channel candidate

- Added owner-polled packet/reply transport boundary: fixed 4096-byte input, one in-flight reply, nonblocking backend contract, explicit partial send accounting, event/deadline checks, buffer wipe, and backend drain before releasing pending response or tearing down radio. Optional local attachment to existing APSTA link. Backend is not implemented yet; no listening socket, HTTP parser, client or startup caller. Backend must later prove AP-only bind and single peer, no retained buffer pointers, bounded execution and confirmed drain. The standard Espressif SoftAP/HTTP Security2 transport is a reference, not verified compatible with this project's custom commands. See https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-reference/provisioning/protocomm.html .
- Four additional combined cases: normal partial sends, ambiguous backend open, invalid send count, pending reply held until backend confirms drain. New code not compiled or executed. Prior d8aa27b2422c9338b1de5b43b3d7e368d00fc0e4 passed 47 GCC ASan/UBSan scenarios and target build in 45783784792; artifact 15099333083, CI digest 00fbd941bc0638b1b410efd273db6ad54885130f04e47cf7b81f9dd521db35c1. Clang absent. Added static RAM and CPU cost require measurement on target.
- No live server, actual credential provisioning, eFuse changes, startup activation, deployment, protected merge or Cloudflare changes.

---

# 2026-11-26 — Read-only protected identity provider candidate

- New versioned setup identity record in separate namespace setup_id within existing explicitly initialized protected room_wifi NVS partition. Read-only application API; no provisioning/write/generation function, no default identity. Strict size/length/padding and nonzero salt/verifier checks are syntax checks only, not evidence of strong or unique per-device credentials. Provider initializes once on a local owner task, validates and wipes probe, rereads only at window prepare, wipes temporary record, assigns local nonzero monotonically increasing SDK session IDs, disables setup after read failure. No startup caller or real phone transport.
- Synthetic host fixture exercises real store/provider, including absent, truncated, invalid version, noncanonical padding, zero salt/verifier, read error, invalid lengths, owner mismatch, repeated prepare and revocation. Candidate not built/tested. Prior f6eac7b9c6f00ac6429ddae98d5d6122d52731d7 passed 28 GCC ASan/UBSan scenarios and target build in 45782880254; artifact 15099481482, CI digest 1f6ebcbd7d0918171517969ca348819955393a40badf91a199d3f45bdfc66e60. Clang absent.
- Espressif documentation: https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-reference/storage/nvs_encryption.html . HMAC-based encrypted NVS is confidentiality protection, not independent authenticated storage or proof of verified boot; real individual identity provisioning and physical trust model require explicit review. No keys, eFuse operations, startup changes, deployment, protected merge or Cloudflare changes.

---

# 2026-11-26 — Owned Security2 identity lifecycle candidate

- Security2 context owns salt (8–64 bytes) and verifier (384 bytes), including persistent SDK descriptor. Never move live context. Wipe on confirmed SDK deletion, retain on ambiguous session open/close. Provider fills inline bytes; temporary window wiped even on prepare failure. No actual identity provider, generation or listener.
- Context tests verify copy independent of original buffers, wipe on creation rejection and successful destroy, and retention on ambiguous failure; combined tests check owned identity and wipe. Candidate not yet built/tested. Prior f56fb72459f40b20f2bef2c306bb3f3207dc2eee passed 27 GCC ASan/UBSan scenarios and target build in 45781923320; artifact 15099405187, CI digest cf35a61e410ad4b6733f647211cdb2cf9cb5a12c2cacf5cd96a8816fe18a9f3d. Clang absent; independent archive verification pending.
- Espressif protocomm docs specify salt and verifier pointer fields; exact pinned SDK source and decrypted-buffer lifecycle still require review. Added memory must be measured on hardware. No startup activation, eFuse writes, protected merge, or deployment.

---

# 2026-11-26 — Verified-save handoff candidate

- APSTA phone policy retains a per-boot committed generation only after authenticated enrollment returns ENROLL_SAVED (write and matching readback). Link snapshots the prior receipt before opening a window. On complete transport/radio teardown only, it reports new verified commit, no save, or uncertain storage. The owner promotes missing profile to valid only for a verified commit, reloads protected storage before STA, and quarantines an uncertain write/readback or invalid reload. Loss of the final response cannot undo verified persistence. No new packet or listener API in production.
- Test-only fixture includes the actual private link translation unit; five new combined scenarios exercise successful first save, lost final response while borrowed buffer remains pending, write failure, readback failure, and reload failure. The cryptographic SDK, radio, and protected store are mocks. New candidate not built/tested yet. Prior 46aeda41abdf79817156efe4ed8ed2cbad383176 passed 22 GCC ASan/UBSan scenarios and target build in 45781113824, artifact 15098671233, CI digest 2978a5539e5d417edd73d20df911ef37e54ad5efce82fc0b22d7cb3b9a7695c4. Clang absent, independent archive verification pending.
- Remaining: identity/provider lifetime, real phone transport and sender drain, SDK decrypted-buffer review, startup integration and physical testing. No activation, eFuse changes, deployment, protected-branch merge or Cloudflare changes.

---

# 2026-11-26 — Protected missing-profile first boot candidate

- Added optional missing-profile initialization in the single network owner and actual APSTA link. It independently reads the already-initialized protected store before initializing and again before opening a missing-profile setup window. Valid, corrupt, unavailable records never authorize automatic setup. Exactly one automatic opportunity on the first owner step; readiness arriving later cannot open AP until a physical BOOT gesture. The link passes missing/valid profile and physical authorization explicitly to the existing session policy. The ordinary valid-profile STA startup remains unchanged, with no provider or live listener.
- Added six new shared-driver test cases: first_normal, first_late, first_error, first_unavailable, first_valid, first_changed; tests check first-window expiry, no automatic reopen, physical reopen and no connection without a stored profile. Candidate not built or tested yet. Authenticated save-to-STA from a missing profile is NOT wired; identity provider, sender lifetime, physical validation and phone transport remain blockers.
- Previous source 92b1d4156ec5d3fdc6074a22af88853723ab69ae passed 16 GCC ASan/UBSan scenarios and ESP32-C3 build in 45780310660, artifact 15098680102, digest 6f8d1ea09a6c68c009c9994e7c9ffe44313ca3977930251ae2e2ec84af3683cd (CI digest, independent verification pending). Clang absent.
- No eFuse/key generation, startup activation, protected branch merge, deployment, paid CI or Cloudflare changes.

---

# 2026-11-26 — Combined real-adapter host integration fixture

- Added a shared SDK mock for actual room_wifi_idf, room_wifi_apsta_link, room_apsta_task, events, session and Security2 transport adapter; the cryptographic SDK remains a synthetic mock, no network/phone traffic. The single driver mock asserts exclusive init, complete cleanup and barrier order. Six independent process cases: disconnect, expiry, revoked readiness, unavailable provider, rejected prepare and partial AP start; seventh case stalls STA cleanup and confirms quarantine. The test also checks protected profile reload and disabled AP event capture during STA.
- New fixture not yet compiled or executed. Previous c5a7f63629ba9df7f879106e59f4f19643694b06 target build and nine GCC ASan/UBSan scenarios passed in 45779558292, artifact 15097602319, CI digest 274fcfd4da7b65eceb17dc02f38aab1c754b43aef8e77b6ba75588af66e38046. Clang unavailable in CI; independent archive verification pending.
- No production source changes, startup activation, real identity material, phone listener, eFuse changes, merge or deployment. Remaining: actual provider/lifetime, first-boot, phone transport and physical testing; MQTT follows.

---

# 2026-11-26 — Single network owner adapter candidate

- Optional, uncalled STA/APSTA binding: the existing wifi_owner policy dispatches both real adapters, and the STA adapter binds to its actual task handle. The optional APSTA link wraps the real persistent room_apsta_task, only accepts a trusted local provider after STA initialization and before first radio start, and does not expose any packet or listener API. Physical BOOT remains the only manual setup trigger. Normal startup is unchanged and supplies no provider. Missing/corrupt first-boot setup is still not wired.
- APSTA ownership is claimed before start (including partial failure); STA cannot restart while APSTA cleanup is incomplete. Shared 5 s cleanup deadline quarantines the Wi-Fi owner on timeout, with no forced reset. Setup deadline remains bounded by the original BOOT request; cancellation/readiness loss is handled by the owner. With the optional binding, credentials are wiped on STA cleanup and reloaded from the protected store before a new STA start. No new key generation or eFuse changes.
- Added a real STA adapter / mock APSTA callback regression for physical BOOT, both barriers, no overlap, cancellation, and profile reload; actual APSTA adapter link is compiled in the target but still needs its own combined SDK integration fixture. New code and tests have not yet been built or executed. Prior static queue commit 8f228ceec291d75ea06adb9be76ef03834b4f907 passed target build and all five GCC tests in run 45778698977.
- Still required: combined adapter fixture, verified identity provider and sender/credential lifetime, protected storage and first-boot handling, real DHCP freshness, heap/stack and physical Wi-Fi tests, and then MQTT. No startup activation, live listener, deployment, merge to protected PR #17, dependency changes or paid CI.

---

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
