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
