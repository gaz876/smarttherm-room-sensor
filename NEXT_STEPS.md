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
