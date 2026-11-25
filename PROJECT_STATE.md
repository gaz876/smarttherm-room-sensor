# Project state

As of 2026-11-25. Standalone experimental C++17 prototype, not production firmware.

## Implemented in safety-control
- SafetyControl: mode transitions, sample age, sequence and session checks; authenticated transport freshness is assumed, not implemented.
- WiredSensors: CRC and conversion flags, age, physical bounds and supply/return delta.
- ReserveControl: weather target, previous target hold with valid wired readings, fixed fallback.
- HeatingLimits: provisional active CH range 40–82 °C, configurable narrower range.
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
