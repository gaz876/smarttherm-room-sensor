# Changelog
Initial standalone SafetyControl prototype and synthetic host tests.

Clarified wired sensor validation and diagnostic-only flame telemetry requirements; no executable changes.

Added wired reading validation and bounded reserve setpoint selection with standalone synthetic tests.

Added HeatingControl and integration test covering nine scenarios and six accepted room samples.

Added and verified standalone recovery setpoint slew limiter and nine assertions.

2026-11-25: Recorded user-reported 40 °C front-panel minimum and provisional 40 °C active CH setpoint floor pending OpenTherm verification. Documentation only.

2026-11-25: Added provisional 82 °C absolute software ceiling for active CH setpoints, alongside the 40 °C floor. Documentation only; OpenTherm range remains unverified.

2026-11-25: HeatingLimits integrated into ReserveControl and RecoveryRamp; seven host test programs passed C++17 strict warnings and nonrecovering UBSan. Provisional 40–82 C limits; no hardware actuation or CI.
