# Project state

As of 2026-11-25. Standalone experimental C++17 prototype, not production firmware.

## Implemented in safety-control
- SafetyControl: mode transitions, sample age, sequence and session checks; authenticated transport freshness is assumed, not implemented.
- WiredSensors: CRC and conversion flags, age, physical bounds and supply/return delta.
- ReserveControl: weather target, previous target hold with valid wired readings, fixed fallback.
- HeatingLimits: provisional active CH range 40–82 °C, configurable narrower range.
- RecoveryRamp: standalone bounded slew limiter.
- HeatingControl: combines safety, wired validation and reserve selection. NORMAL intentionally blocked pending PID handoff.

## Verification
Seven host test files are published. Earlier reports record C++17 strict warnings and UBSan passes; full seven-program execution has not been independently reconfirmed in the latest review. No CI or physical actuation validation.
40 °C is the user-reported front-panel minimum; actual OpenTherm limits remain unverified. No heat demand must mean CH off, not a 40 °C demand. Final command enforcement is pending. Preserve DHW and boiler protections.

## Pending
Final OpenTherm command guard, authenticated freshness across restarts, adaptive reserve adjustment, PID handoff, integration with original SmartTherm 0.8.4.16 and bench validation. No production deployment.

## Repository baseline
Last code-and-journal baseline before this documentation cleanup: 13f07db4a04d99738e0ef984ee3fb4703cad7ed0.
