## 2026-11-25 — Independent full regression

Nine existing host test executables rebuilt and run, nine exit 0; 17/17 Git blob hashes matched. Public run artifact lists empty. Documentation only; no executable changes or new CI.

---

# Independent partial audit — 2026-11-25

Re-read historical public job logs and matched all 17 source/test blobs to the last CI baseline. Actual local SafetyControl test and independent guard/integration probes executed; observations show NORMAL continuity and restart replay limitations. Documentation only, no executable changes. Full audit remains open.

---

# Current checkpoint — 2026-11-25

2026-11-25: Current ceiling is 80 °C for active CH setpoints (40 °C floor; narrower valid configuration allowed). Supersedes historical 82 °C decision. Actual OT range and hardware remain unverified. PR 6 merged at 11f8fdc82d1a240e14a939f0f26618c6e8b8d3d9; tested source 83f71e796a13b4b1b9712dc5d718ae602a4c8f8c; CI workflow 00b7ce1f6a2f185958a61e06c512f85b7b35a236, run 45539130805, job 138953026312, PASS 9/9, GCC 13.3.0, Ubuntu 24.04.5, UBSan, Metadata read only. Full job log inspected. No production or hardware changes. Secret settings confirmed unchanged by owner; API inspection unavailable. Next: private PID handoff and MQTT/OT integration review.


PR 3 merged into experimental safety-control at 932677fe9c1a8330c1708bb874213eac7009c913; PR 4 enabled automatic pinned-source CI on main workflow changes. Run 45531837467 PASS 9/9, full logs inspected, Metadata read only, zero artifacts, no apparent secret leakage. GitHub-first policy applies; no production changes. Earlier local test claims are superseded by verified CI. Historical entries below are retained, not current status.

---
## Historical records (superseded where conflicting)

# Changelog
Initial standalone SafetyControl prototype and synthetic host tests.

Clarified wired sensor validation and diagnostic-only flame telemetry requirements; no executable changes.

Added wired reading validation and bounded reserve setpoint selection with standalone synthetic tests.

Added HeatingControl and integration test covering nine scenarios and six accepted room samples.

Added and verified standalone recovery setpoint slew limiter and nine assertions.

2026-11-25: Recorded user-reported 40 °C front-panel minimum and provisional 40 °C active CH setpoint floor pending OpenTherm verification. Documentation only.

2026-11-25: Added provisional 82 °C absolute software ceiling for active CH setpoints, alongside the 40 °C floor. Documentation only; OpenTherm range remains unverified.

2026-11-25: HeatingLimits integrated into ReserveControl and RecoveryRamp; seven host test programs passed C++17 strict warnings and nonrecovering UBSan. Provisional 40–82 C limits; no hardware actuation or CI.

2026-11-25: Reconciled project state and next steps with published baseline 13f07db. Historical test-pass reports retained; full seven-test rerun remains pending independent confirmation. OpenThermGuard not implemented. Documentation only.

2026-11-25: Published standalone RecoveryAdapter and adapted test to 40 C floor. All eight host tests passed strict C++17 and nonrecovering UBSan. No actual PID handoff or actuation.


2026-11-25: Independent GitHub-hosted CI verification PASS 8/8, run 45528121420, job 138916424211, audited source da533974ded6bc8acbb8ad164f64d1f137bd0140, workflow cf14dec44d00150adc405cc91654778545477938. Ubuntu 24.04.5, GCC 13.3.0, C++17 strict warnings and nonrecovering UBSan. Full log inspected: all eight PASS, Metadata read only, zero artifacts, no apparent secret leakage. No hardware or production validation. Next: standalone OpenThermGuard.

2026-11-25: Standalone OpenThermGuard candidate in safety/guard-20261125, code/test commit f0ac3314d07b991f4913174b7c98614d5664f50c. Local GCC 14.2 C++17 strict warnings and nonrecovering UBSan PASS, 14 synthetic cases. Published blobs match tested files. Decision only; no transport, DHW changes or hardware actuation. Provisional limits remain unverified on hardware. Next: review PR and independent nine-program CI.

2026-11-25: Verified GitHub Actions push run 45531837467, job 138928787324, source f0ac3314d07b991f4913174b7c98614d5664f50c, workflow 7fb898d28ce5f001008eedfa46042f9d82e1ad3e (PR 4). PASS 9/9 including guard, GCC 13.3.0, Ubuntu 24.04.5, strict warnings and nonrecovering UBSan. Full log inspected, Metadata read only, zero artifacts, no apparent secret leakage. Earlier local PASS claim lacks independent execution evidence; this CI supersedes it. GitHub-first: perform feasible development, testing, audit and checkpoints in GitHub; local only when safe GitHub execution is unavailable. No hardware validation.
