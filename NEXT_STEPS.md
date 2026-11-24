# Next steps
Implement and test authenticated freshness across restarts, reserve setpoint and PID recovery. Audit CI separately before enabling it. Hardware integration requires bench validation.
Host test: `g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=undefined -Isrc tests/test.cpp -o /tmp/safetytest && /tmp/safetytest`.

## Reserve control requirements
- Preserve original PID and weather-compensation algorithms.
- Without room and outdoor measurements, use wired supply and return sensors, temperature difference and trends, and available OpenTherm flame/modulation telemetry.
- Start from the last valid bounded heating setpoint; use a configured fixed fallback if unavailable or wired sensors fail.
- Do not infer room temperature or heat demand from delta-T alone. Adaptive coefficients and limits require bench validation; no automatic adaptive actuation is approved yet.
- Preserve boiler protections and DHW behavior. No relay switching for room sensor loss.

## Wired sensor validation
- Validate DS18B20 CRC, conversion completion, measurement age and configured physical limits before computing delta-T.
- Negative delta-T alone is not a sensor fault.
- Flame modulation and cycling are diagnostic only; never use them directly to select the reserve setpoint.
- Compare boiler supply telemetry with wired supply for diagnostics when supported.

Integrate WiredSensors validity with ReserveControl; implement recovery and authenticated freshness. Adaptive correction requires physical validation.

Next: implement verified PID recovery and transport freshness; adaptive wired control remains unvalidated. Integration test: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=undefined -Isrc tests/test_heating.cpp -o /tmp/test_heating && /tmp/test_heating.

Integrate recovery ramp with verified PID initialization and handoff; keep NORMAL blocked until validated. Test: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all -Isrc tests/test_ramp.cpp -o /tmp/test_ramp && /tmp/test_ramp.

## Mandatory heating setpoint constraint
- Enforce a provisional 40 °C minimum active CH setpoint across reserve, PID, weather compensation and recovery; validate upper limit and reject incompatible configuration.
- Keep CH disabled when no heat demand; preserve DHW and boiler protections.
- Revise synthetic tests and models that use active setpoints below 40 °C. Verify boundary and handoff behavior.
- Verify actual OpenTherm accepted minimum on the physical Buderus Logamax U072-12K before revising the provisional floor.

## Upper limit requirement
Enforce 40–82 °C active CH range in all control paths, with a configurable lower user ceiling within that range. Reject incompatible configuration; validate final command before OpenTherm transmission. Test both boundaries, out-of-range requests and no-demand behavior. Verify physical boiler and OpenTherm limits before production deployment.
