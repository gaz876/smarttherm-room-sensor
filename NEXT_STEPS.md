# Next steps
Implement and test authenticated freshness across restarts, reserve setpoint and PID recovery. Audit CI separately before enabling it. Hardware integration requires bench validation.
Host test: `g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=undefined -Isrc tests/test.cpp -o /tmp/safetytest && /tmp/safetytest`.

## Reserve control requirements
- Preserve original PID and weather-compensation algorithms.
- Without room and outdoor measurements, use wired supply and return sensors, temperature difference and trends, and available OpenTherm flame/modulation telemetry.
- Start from the last valid bounded heating setpoint; use a configured fixed fallback if unavailable or wired sensors fail.
- Do not infer room temperature or heat demand from delta-T alone. Adaptive coefficients and limits require bench validation; no automatic adaptive actuation is approved yet.
- Preserve boiler protections and DHW behavior. No relay switching for room sensor loss.
