# Independent partial audit — 2026-11-25

At source bf2585a82e6bb210fd6c9881268590bd6fa666e0, all 17 src/tests blobs match GitHub CI source 83f71e796a13b4b1b9712dc5d718ae602a4c8f8c. All four historical public job logs reread, last run 45539130805 / job 138953026312 reports 9/9 actual executions. Locally verified eight header blobs and tests/test.cpp blob 5e559a6b9a7c678e4c3e39953391508871c51c7e, GCC/G++ 14.2.0, strict C++17 nonrecovering UBSan, 10 s timeout: original SafetyControl test exit 0. Independent integration probe exit 0: 1,203 guard decisions plus invalid inputs and reserve/recovery adapter scenarios (probe SHA256 bcc42a5e357337904875cf494e11ed2a1449f4822a288669b7a9942befb549e1). Independent observation probe exit 0 (SHA256 35c4ae19021f7ce90ad80a2267d3ef173879c062cd948d1f8b87f2891f482f4f): NORMAL persists after explicit NaN and after a new sample following a 50 s gap, old tuple accepted after object recreation with fresh=true. These are observed limitations, not a claim of physical hazard or a failed existing test; define intended failover policy and implement persistent transport freshness before integration. HeatingControl itself passes recoveryDone=false and blocks NORMAL. No hardware, target, live OT, or actual PID verification. No new CI; admin settings and full history audit pending. No new functionality.

---

# Current state — 2026-11-25

2026-11-25: Current ceiling is 80 °C for active CH setpoints (40 °C floor; narrower valid configuration allowed). Supersedes historical 82 °C decision. Actual OT range and hardware remain unverified. PR 6 merged at 11f8fdc82d1a240e14a939f0f26618c6e8b8d3d9; tested source 83f71e796a13b4b1b9712dc5d718ae602a4c8f8c; CI workflow 00b7ce1f6a2f185958a61e06c512f85b7b35a236, run 45539130805, job 138953026312, PASS 9/9, GCC 13.3.0, Ubuntu 24.04.5, UBSan, Metadata read only. Full job log inspected. No production or hardware changes. Secret settings confirmed unchanged by owner; API inspection unavailable. Next: private PID handoff and MQTT/OT integration review.


Goal: prepare experimental extensions for original SmartTherm 0.8.4.16 with a wireless room sensor; preserve original PID, DHW and boiler protections. No original firmware modification or hardware deployment yet.

Target architecture: SHT40 → MQTT → freshness/validity checks → original PID or bounded reserve → recovery ramp/PID handoff → OpenTherm command guard. Integration is pending.

| Module | Verified scope |
|---|---|
| SafetyControl | Age, sequence, session and mode checks; authenticated transport pending |
| WiredSensors | CRC, conversion, age and physical bounds |
| ReserveControl | Weather target, last valid target or fixed fallback; adaptive reserve pending |
| HeatingLimits | Provisional active CH 40–80 °C; actual OT limits pending |
| RecoveryRamp | Bounded slew |
| RecoveryAdapter | Cancellation and preparation flag; actual PID initialization pending |
| HeatingControl | Combined prototype; NORMAL intentionally blocked |
| OpenThermGuard | Decision only: fault/off/invalid/active; no transport or actuation |

CI PASS 9/9: run 45531837467, job 138928787324, source f0ac3314d07b991f4913174b7c98614d5664f50c, workflow 7fb898d28ce5f001008eedfa46042f9d82e1ad3e. GCC 13.3.0, Ubuntu 24.04.5, strict C++17 and UBSan. PR 3 merged into safety-control at 932677fe9c1a8330c1708bb874213eac7009c913. No hardware validation. Room sensor dev at 5f5673f24183033d06b4d902c417e8062d9f9ad9: codec host test reported PASS; target build, MQTT and full firmware pending.

---
## Historical records (superseded where conflicting)

# Project state

As of 2026-11-25. Standalone experimental C++17 prototype, not production firmware.

## Implemented in safety-control
- SafetyControl: mode transitions, sample age, sequence and session checks; authenticated transport freshness is assumed, not implemented.
- WiredSensors: CRC and conversion flags, age, physical bounds and supply/return delta.
- ReserveControl: weather target, previous target hold with valid wired readings, fixed fallback.
- HeatingLimits: provisional active CH range 40–80 °C, configurable narrower range.
- RecoveryRamp: standalone bounded slew limiter.
- HeatingControl: combines safety, wired validation and reserve selection. NORMAL intentionally blocked pending PID handoff.

- RecoveryAdapter: standalone recovery cancellation and PID-prepared flag; not actual PID initialization.

## Verification
Eight host test files are published. All eight passed local C++17 strict warnings and nonrecovering UBSan on 2026-11-25. No CI or physical actuation validation.
40 °C is the user-reported front-panel minimum; actual OpenTherm limits remain unverified. No heat demand must mean CH off, not a 40 °C demand. Final command enforcement is pending. Preserve DHW and boiler protections.

## Pending
Final OpenTherm command guard, authenticated freshness across restarts, adaptive reserve adjustment, PID handoff, integration with original SmartTherm 0.8.4.16 and bench validation. No production deployment.

## Repository baseline
Last code-and-journal baseline before this documentation cleanup: 13f07db4a04d99738e0ef984ee3fb4703cad7ed0.


2026-11-25: Independent GitHub-hosted CI verification PASS 8/8, run 45528121420, job 138916424211, audited source da533974ded6bc8acbb8ad164f64d1f137bd0140, workflow cf14dec44d00150adc405cc91654778545477938. Ubuntu 24.04.5, GCC 13.3.0, C++17 strict warnings and nonrecovering UBSan. Full log inspected: all eight PASS, Metadata read only, zero artifacts, no apparent secret leakage. No hardware or production validation. Next: standalone OpenThermGuard.

2026-11-25: Standalone OpenThermGuard candidate in safety/guard-20261125, code/test commit f0ac3314d07b991f4913174b7c98614d5664f50c. Local GCC 14.2 C++17 strict warnings and nonrecovering UBSan PASS, 14 synthetic cases. Published blobs match tested files. Decision only; no transport, DHW changes or hardware actuation. Provisional limits remain unverified on hardware. Next: review PR and independent nine-program CI.

2026-11-25: Verified GitHub Actions push run 45531837467, job 138928787324, source f0ac3314d07b991f4913174b7c98614d5664f50c, workflow 7fb898d28ce5f001008eedfa46042f9d82e1ad3e (PR 4). PASS 9/9 including guard, GCC 13.3.0, Ubuntu 24.04.5, strict warnings and nonrecovering UBSan. Full log inspected, Metadata read only, zero artifacts, no apparent secret leakage. Earlier local PASS claim lacks independent execution evidence; this CI supersedes it. GitHub-first: perform feasible development, testing, audit and checkpoints in GitHub; local only when safe GitHub execution is unavailable. No hardware validation.
