# I2C recovery candidate — 2026-11-26

Experimental source bd67087c9107a9056214fe1e6d008012318a2a64, based on dev 4a44c5512afa8d3ea0c96d0db4117c907a854ec8. Startup bus creation and device-add failures now retry indefinitely with nominal 1/2/4/8/16/30 s backoff (30 s cap, tick rounding). Three consecutive I/O failures schedule bus reset, device removal, bus deletion and recreation. Partial resource cleanup is retried without allocating a new bus while ownership remains. CRC/timing errors are not counted as I/O failures. A valid measurement clears backoff; reinitialization clears old measurement state. Invalid static configuration still stops deliberately. Existing sample cadence and correction unchanged. Reviewed official ESP-IDF v5.5.6 I2C API and implementation at 880a203ae603d08a94d06aa01e0d6761958e5e9e, i2c_master.c blob 781f4bb078c97143cd34ef1f72a555a3f3ce725c. Host test script actually ran with GCC 14.2.0 and Clang 17.0.0, C11 strict warnings and nonrecovering UBSan; three programs each, six exit 0, script exit 0. New tests cover partial init, capped retry, cleanup faults, consecutive I/O and 1000 synthetic recovery cycles. Ten local Git blobs matched repository: adapter 39d92f84202d78f7a1dbbf1ac3c7ebc601702eea, retry header f616700a5bb757213472bda41623d38119b7bc92, new test 45c6edb27ad9a0399c628bc8bd16388f72cc9d1e, runner 0cb339ba04f458fd0f897e0b2ca40f7af80cf49d, measure C/H 8296297daaeb3c803fdaef07c5f06b79adc23aaa / 713af78bad9e27af4e4bc40decf879e4990c92ca, codec C/H cc9dc7317de93b38ff49a5dba98af96d3c3c4385 / d40ace0c443e6d96e7a7eaa356e491494f330395, existing tests f7c14df0d8e801ec9e5bffc1451f106a7b94cfc4 / 5ff3c557559a7327c74967a903a84868483cbe5b. Adapter main.c was hash-checked and reviewed, not compiled for target. No physical validation, no proof of driver-level resource recovery on hardware or of stuck-line recovery. SDK/compiler for target and Docker absent locally. No new CI, dependency additions, network, OTA, private inputs or heating changes. Existing audit gates remain.

---

# First successful ESP32-C3 target build — verified 2026-11-26

Public main f7d7fe764ed6d09a195ed349cd17a15b9180044a, workflow blob 4b2f474b0f0d3fc40f369eaaea5c49f62a59b7b4, pinned source 08f487175b6995864285e420a72f0092a102c68f. Run 45632347131 / job 139268608712 completed success on 2026-11-25 UTC (14:17:02–14:18:55 log timestamps). All three job steps succeeded; full log retrieved (188594 characters, 1577 lines). All nine build-input Git blobs logged by the runner match current dev. Metadata read only, no apparent secret exposure in inspected output. Official image digest sha256:851f136a2c09cf17c13e952b9975606d7d1050be1480b1fd82c7ea4bce78cfb3, SDK v5.5.6 @ 880a203ae603d08a94d06aa01e0d6761958e5e9e, GCC esp-14.2.0_20260121 / 14.2.0 (compiler SHA256 789918490cee5a48112cae580f3279c08a26d37fbf738b8655b4b7dbe11f9957), Python 3.12.3, CMake 3.30.2, Ninja 1.11.1. Pip check passed; full installed package list and recursive submodule inventory in run log. Actual idf.py set-target esp32c3, build (1003/1003), size and file checks passed, ESP32C3_BUILD_OK printed. No compiler/CMake warning or error marker found in inspected log; expected nonfatal git rev-parse message because project inputs are archived without .git. 105 configured application components, 21 bootloader components; this is not the exact linked-component closure.

Outputs recorded by runner:
- room_sensor.bin 194848 bytes, SHA256 3806096a99740d9a661686b6b58d31e116960b335d561ce5f9095731f74d59d4
- room_sensor.elf SHA256 ad224dd8817e5878e586c9004d5ec87701833fc1fb4ac13e64260267ac149aad
- room_sensor.map SHA256 20c4a51f22137d8fc27719424ba9792c5aee94e2b797fa1647a872e4769d4354
- bootloader.bin 20624 bytes, SHA256 3bb6bd344c160da155c3b9d56adf6a38d279e4d5f5973d518145656e3f7cb2cc
- partition-table.bin 3072 bytes, SHA256 7f00b6c042a89b15b0cac534f82ed988caf29278ff5700b0c511eb1b5bb7c820

Size report: image 194514 bytes before padding, static DRAM 60158/321296 bytes (18.72%), remainder 261138 bytes not measured runtime heap. Factory app 1 MiB, app binary 18.58% occupied, 853728 bytes free. Generated partition table verified in log: nvs 0x9000 24K, phy_init 0xf000 4K, factory 0x10000 1M. Configured flash 4MB; no OTA slots. Artifact API returned empty list by design, so bytes were not downloaded or independently hashed; do not claim downloadable firmware or independent binary verification. No flash, hardware, network integration or heating validation. Full dependency/advisory audit and hardware gates remain open. No code or workflow change in this checkpoint.

---

# ESP32-C3 CI input-copy fix — 2026-11-25

First target run 45630874212 / job 139263403475 failed exit 1 before idf.py: cp -a attempted ownership preservation in the capability-restricted container. Full job log retrieved (20948 characters); runner Ubuntu 24.04.5, GITHUB_TOKEN Metadata read only, nine source blobs matched known baseline, image digest sha256:851f136a2c09cf17c13e952b9975606d7d1050be1480b1fd82c7ea4bce78cfb3 pulled successfully. SDK v5.5.6, pinned SDK HEAD and tracked diff checks passed, 27 recursive submodule status entries with no mismatch flags, GCC esp-14.2.0_20260121 / 14.2.0, compiler executable SHA256 789918490cee5a48112cae580f3279c08a26d37fbf738b8655b4b7dbe11f9957, Python 3.12.3, CMake 3.30.2, Ninja 1.11.1, pip check no broken requirements; installed Python inventory in original log. This is dependency compatibility evidence, not full advisory clearance. No compiled firmware or hardware test.

Local capsh --drop=cap_chown reproduction with input owned by UID/GID 1001: cp -a exit 1; cp -R --no-preserve=ownership exit 0 and cmp verified file content. Public PR #9 merged as f7d7fe764ed6d09a195ed349cd17a15b9180044a; workflow blob 4b2f474b0f0d3fc40f369eaaea5c49f62a59b7b4, exactly one changed line, source/image pins, container restrictions and triggers unchanged. New run 45632347131 queued at check. No production changes, no artifact upload, no private build. Rollback by reviewed revert of PR #9 if necessary.

---

# Isolated ESP32-C3 CI — 2026-11-25

Public main 5db5202250e5ddb62639c6f290095633e4d21cb0, PR #8 merged; workflow .github/workflows/esp32c3-build.yml blob 6e95cc2764d73b40cbfec53cd9ac7cabcd4fa658. Source fixed at 08f487175b6995864285e420a72f0092a102c68f. Official Espressif linux/amd64 IDF v5.5.6 manifest sha256:851f136a2c09cf17c13e952b9975606d7d1050be1480b1fd82c7ea4bce78cfb3 (https://hub.docker.com/layers/espressif/idf/v5.5.6/images/sha256-851f136a2c09cf17c13e952b9975606d7d1050be1480b1fd82c7ea4bce78cfb3). Reviewed SDK Dockerfile and entrypoint at pinned SDK commit, project build input files and workflow diff; YAML parsed with PyYAML BaseLoader, bash -n exit 0. Owner reconfirmed absence of public user tokens/secrets, previously checked three times; administrative permissions/environment endpoints remain inaccessible, not independently verified. Workflow uses permissions: {}, no secrets/environment references, no third-party actions, no private repo, standard ubuntu-24.04, 25 min timeout, manual or exact main workflow-path push only. Container digest pinned, no network, capabilities dropped, no-new-privileges, only read-only archive of public build inputs mounted. No cache, artifact upload, OTA or deployment. Actual image package/advisory audit still incomplete; this isolated experiment is not production clearance. Rollback by reviewed revert of the workflow commit.

Run 45630874212, job 139263403475, first run in_progress at last check; setup success, build step in_progress. No target build success or binaries confirmed. Workflow intentionally logs output checksums without uploading binaries pending artifact audit. Heating frozen.

---

# Build input review — 2026-11-25

Baseline dev 5e405260e7c8d06625dcfb11cd01a1e3b23b5a9c; SDK v5.5.6 @ 880a203ae603d08a94d06aa01e0d6761958e5e9e. Read the official live Python constraints via web at https://dl.espressif.com/dl/esp-idf/espidf.constraints.v5.5.txt (not a downloaded/hashed lockfile). Core requirements blob fbd46e5874a49481f43b3ff168a978b06915631f and constraints include version ranges rather than a fully pinned environment; actual resolved versions, transitive dependencies, wheel hashes and advisory review remain pending. Compiler manifest blob baca9b017dd8f1a2dc66d65f8d5a3b5bdfa1abd6: recommended Linux x86_64 riscv32-esp-elf esp-14.2.0_20260121, archive size 292412032 bytes, expected SHA256 b3fce4b04dd15a9f0ed1c209b5a2f389c04eff92ed85e4cc0ebc275b7d95e95a. Actual archive not retrieved or verified.

Read project build and measurement sources. No COMPONENTS override or MINIMAL_BUILD; documented default includes all discovered SDK components, with actual target configuration/linked closure still unknown. The default partition choice is SINGLE_APP (SDK Kconfig.projbuild blob c7ef8b073fadeffaa53a6b2c3bb9ca9512bf84e4): normal singleapp CSV blob 5156e827b4342ae4ca7bf3ee9e8a7e101e9ec45f has 1 MiB factory app, NVS and PHY, no OTA slots. Project sdkconfig.defaults selects 4 MB flash only; actual generated partition table still unverified. Do not infer OTA readiness from flash size. References: https://docs.espressif.com/projects/esp-idf/en/v5.5.6/esp32c3/api-guides/build-system.html and https://docs.espressif.com/projects/esp-idf/en/v5.5.6/esp32c3/api-reference/peripherals/i2c.html .

Current isolated environment: IDF_PATH empty, idf.py, target compiler, Docker/Podman absent; CMake/Ninja/Git/Python present. DNS lookups for github.com, dl.espressif.com and pypi.org exited 2. Alternate direct download of constraints failed, although web text retrieval succeeded. No SDK installed, no target compilation or host retest, no hardware. Administrative CI gate remains open. No functional changes, paid operations or deployment.

---

# SDK provenance extension — 2026-11-25

Public dev baseline 79b210eca3d01093bc34a08fb643dc234322427b; tree includes no workflow or external component manifest. Official ESP-IDF v5.5.6 signed commit 880a203ae603d08a94d06aa01e0d6761958e5e9e recursive Git tree returned untruncated with 23 pinned gitlinks. This establishes first-level submodule inventory, not content verification or actual build closure. Relevant pinned entries include heap/tlsf 2867f6883a12920b1969ff9624c0ab0e4185c2ce, lwip/lwip c6f2f878e7b0f86033214b85547d579be43351e3, mbedtls/mbedtls 2b96dd8eebe880f304c69976b3c2fa0c5100cbb6 and mqtt/esp-mqtt 0e9ec171b448db2101ea537300976b7ffe595941 (under components/). Official release asset esp-idf-v5.5.6.zip is 2146536906 bytes; GitHub reports SHA256 d6c08f2bd7f0875c46c65cb5a34f83337bea092c9b0337405f62d1923ed73ddb. Not downloaded or independently verified; archive still requires build tools. Official advisories GHSA-ch63-jxm5-vmvf (lwIP DNS) and GHSA-rjvf-fxv5-x87j (BLE Mesh) identify v5.5.6 as fixed. This is a limited advisory review, not full security clearance. Sources: https://github.com/espressif/esp-idf/releases/tag/v5.5.6 ; https://github.com/espressif/esp-idf/security/advisories ; https://docs.espressif.com/projects/esp-idf/en/v5.5.6/esp32c3/api-guides/build-system.html

Fresh isolated environment: IDF_PATH empty, idf.py, riscv32-esp-elf-gcc, Docker and Podman absent; CMake/Ninja/Git present; three host lookups returned no address and separate download of official v5.5 constraints failed. No target build or new host retest. Public open PR count zero and Actions count four; reviewed both main workflows (manual or main workflow-file push only). Administrative CI gate still open. No source/config, workflow, firmware, OTA or heating change.

---

# ESP-IDF build gate recheck — 2026-11-25

Pinned dev 34635097738ce64221b7b9b5fd777f07d1e848fb. GitHub compare against e433dc0330ca80b6cd97ec432cb8b2db6375404a shows only three journals changed. Reviewed current adapter/build files and official ESP-IDF v5.5.6 commit 880a203ae603d08a94d06aa01e0d6761958e5e9e component manifests: esp_driver_i2c directly requires esp_driver_gpio, esp_pm, esp_ringbuf privately on target; esp_timer and FreeRTOS are SDK components. Project has no external component manifest/lock in its tree. Default build may include unused SDK components; official MINIMAL_BUILD property is available for future reviewed change; not applied. SDK core Python requirements depend on external constraints, including cryptography, esptool and component manager; actual resolved versions/hashes and transitive component/submodule set not yet verified. Official build system: https://docs.espressif.com/projects/esp-idf/en/v5.5.6/esp32c3/api-guides/build-system.html

Fresh environment check: cmake/ninja/git available; IDF_PATH empty, idf.py, riscv32-esp-elf-gcc, Docker and Podman absent from PATH; getent ahostsv4 github.com, dl.espressif.com, pypi.org returned no addresses; direct constraints download failed. No target compilation was possible, no new test claim. Public repository confirmed; four historical Actions runs; no open public PR; main workflows limited to manual runner and safety-host main workflow path. Actions permissions/workflow endpoint rejected by connector, dev branch protection returned 403, secrets/environments and full SDK audit unresolved. No CI publication or run, no source/config changes, no OTA. Prior GPIO/init recovery/partition findings remain.

---

# Measurement regression checkpoint — 2026-11-25

Pinned source e433dc0330ca80b6cd97ec432cb8b2db6375404a. Independent state-machine retest in isolated Linux: all five Git blobs matched (src/sht40.c cc9dc7317de93b38ff49a5dba98af96d3c3c4385, src/sht40.h d40ace0c443e6d96e7a7eaa356e491494f330395, src/room_measure.c 8296297daaeb3c803fdaef07c5f06b79adc23aaa, src/room_measure.h 713af78bad9e27af4e4bc40decf879e4990c92ca, tests/test_room_measure.c f7c14df0d8e801ec9e5bffc1451f106a7b94cfc4). GCC Debian 14.2.0-19 and Clang 17.0.0 (10999b6d034fe318f3d56c83bddb6572593a8bb0), each actual exit 0. Command for each cc: $cc -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=undefined -fno-sanitize-recover=all -Isrc src/sht40.c src/room_measure.c tests/test_room_measure.c -lm -o /tmp/sensor-retest/test; timeout 10s /tmp/sensor-retest/test. Output: PASS: restart, reset/command/read failures, CRC, recovery, cadence, stale, correction, clock rollback, late/early I/O, no catch-up. No hardware or target build. SDK/compiler/Docker/Podman absent; getent hosts github.com dl.espressif.com pypi.org returned no addresses (individual lookups). Existing audit gates remain; no CI requested, no code change.

---

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
