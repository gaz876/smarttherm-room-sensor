# Project state
Experimental standalone state machine; not production firmware. Host C++17 tests passed with strict warnings and UBSan. No workflows, hardware access or network code. Transport authentication, freshness across restarts, PID recovery and reserve setpoint remain unimplemented.

Reserve sensor validation requirements documented. Adaptive control and hardware validation remain pending.

Current update: WiredSensors and bounded ReserveControl prototypes are published. ReserveControl host tests passed (six assertions, C++17, strict warnings, UBSan). Adaptive adjustment and integration remain pending.

HeatingControl integration published. Host GCC 14.2 C++17 strict warnings and UBSan integration test passed. NORMAL intentionally blocked pending PID recovery. No actuation.

RecoveryRamp and test_ramp.cpp published; exact Git blob hashes matched host copies. GCC 14.2 C++17 strict warnings and nonrecovering UBSan test passed. PID handoff remains blocked.
