# Project state
Experimental standalone state machine; not production firmware. Host C++17 tests passed with strict warnings and UBSan. No workflows, hardware access or network code. Transport authentication, freshness across restarts, PID recovery and reserve setpoint remain unimplemented.

Reserve sensor validation requirements documented. Adaptive control and hardware validation remain pending.

Current update: WiredSensors and bounded ReserveControl prototypes are published. ReserveControl host tests passed (six assertions, C++17, strict warnings, UBSan). Adaptive adjustment and integration remain pending.

HeatingControl integration published. Host GCC 14.2 C++17 strict warnings and UBSan integration test passed. NORMAL intentionally blocked pending PID recovery. No actuation.

RecoveryRamp and test_ramp.cpp published; exact Git blob hashes matched host copies. GCC 14.2 C++17 strict warnings and nonrecovering UBSan test passed. PID handoff remains blocked.

## Buderus minimum heating setpoint
User reports a 40 °C minimum heating setpoint on the Buderus Logamax U072-12K front panel. OpenTherm minimum is not yet verified. Pending hardware validation, enforce 40 °C as the minimum active CH setpoint for reserve, PID, weather compensation and recovery. No heat demand means CH request off, not continuous heating at 40 °C. This requirement is documented, not yet implemented.
