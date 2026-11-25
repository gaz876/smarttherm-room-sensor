# Next steps

## Immediate
1. Baseline: eight host tests passed locally; RecoveryAdapter published. Actual PID initialization and integration remain pending.
2. Implement standalone OpenThermGuard and synthetic tests: finite bounded active CH target, valid configuration, no-demand CH off, explicit communication failure; preserve DHW and boiler protections.
3. Audit changes before publication; verify published blobs and update journals.

## Subsequent
- Integrate recovery ramp with verified original PID initialization, derivative history and timer handling; keep NORMAL blocked until validated.
- Implement authenticated transport freshness across restarts and replay protection.
- Develop reserve adjustment using wired supply/return readings and trends. Start from last valid bounded target; use fixed fallback if unavailable or wired readings fail. Delta-T alone cannot establish room temperature or demand. Negative delta-T alone is not a fault. Flame modulation and cycling are diagnostic only. Compare boiler and wired supply telemetry when supported.
- Preserve original PID and weather compensation, DHW behavior and boiler protections. No relay switching for room sensor loss.
- Integrate with original SmartTherm 0.8.4.16 only after source review; validate actual OpenTherm limits on Buderus U072-12K. Provisional active CH range 40–82 °C; user maximum may be lower within this range.
- Bench validation before production or adaptive actuation. Audit CI separately before enabling workflows.

## Host test command
```sh
for f in tests/*.cpp; do
  g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all -Isrc "$f" -o /tmp/safetytest && /tmp/safetytest || exit 1
done
```
