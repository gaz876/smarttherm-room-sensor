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
