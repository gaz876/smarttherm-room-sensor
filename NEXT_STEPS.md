# Current next steps — 2026-11-25

2026-11-25: Current ceiling is 80 °C for active CH setpoints (40 °C floor; narrower valid configuration allowed). Supersedes historical 82 °C decision. Actual OT range and hardware remain unverified. PR 6 merged at 11f8fdc82d1a240e14a939f0f26618c6e8b8d3d9; tested source 83f71e796a13b4b1b9712dc5d718ae602a4c8f8c; CI workflow 00b7ce1f6a2f185958a61e06c512f85b7b35a236, run 45539130805, job 138953026312, PASS 9/9, GCC 13.3.0, Ubuntu 24.04.5, UBSan, Metadata read only. Full job log inspected. No production or hardware changes. Secret settings confirmed unchanged by owner; API inspection unavailable. Next: private PID handoff and MQTT/OT integration review.


1. Review original SmartTherm 0.8.4.16 PID state, integral, derivative history and timing; design and test bumpless recovery. Keep NORMAL blocked until validated.
2. Implement authenticated freshness/replay protection across restarts and adaptive reserve using valid wired trends; delta-T alone is not room demand.
3. Integrate guard with actual command path, never send zero sentinel as active target; preserve CH off, DHW and boiler protections.
4. Develop room sensor in dev: ESP-IDF target build, Wi-Fi, MQTT authorization, freshness, exact topics and full firmware.
5. Review integration with original firmware; verify actual OT limits and hardware on bench before any deployment. Provisional 40–80 °C is not a verified OT range.
6. Use GitHub for feasible work, pinned audited CI, synchronize journals public → private after each checkpoint. No paid resources or production deployment without approval.

Completed: standalone guard and nine-program CI (run 45531837467). Historical tasks below are not active instructions.

---
## Historical records (superseded where conflicting)

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
- Integrate with original SmartTherm 0.8.4.16 only after source review; validate actual OpenTherm limits on Buderus U072-12K. Provisional active CH range 40–80 °C; user maximum may be lower within this range.
- Bench validation before production or adaptive actuation. Audit CI separately before enabling workflows.

## Host test command
```sh
for f in tests/*.cpp; do
  g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all -Isrc "$f" -o /tmp/safetytest && /tmp/safetytest || exit 1
done
```


2026-11-25: Independent GitHub-hosted CI verification PASS 8/8, run 45528121420, job 138916424211, audited source da533974ded6bc8acbb8ad164f64d1f137bd0140, workflow cf14dec44d00150adc405cc91654778545477938. Ubuntu 24.04.5, GCC 13.3.0, C++17 strict warnings and nonrecovering UBSan. Full log inspected: all eight PASS, Metadata read only, zero artifacts, no apparent secret leakage. No hardware or production validation. Next: standalone OpenThermGuard.

2026-11-25: Standalone OpenThermGuard candidate in safety/guard-20261125, code/test commit f0ac3314d07b991f4913174b7c98614d5664f50c. Local GCC 14.2 C++17 strict warnings and nonrecovering UBSan PASS, 14 synthetic cases. Published blobs match tested files. Decision only; no transport, DHW changes or hardware actuation. Provisional limits remain unverified on hardware. Next: review PR and independent nine-program CI.

2026-11-25: Verified GitHub Actions push run 45531837467, job 138928787324, source f0ac3314d07b991f4913174b7c98614d5664f50c, workflow 7fb898d28ce5f001008eedfa46042f9d82e1ad3e (PR 4). PASS 9/9 including guard, GCC 13.3.0, Ubuntu 24.04.5, strict warnings and nonrecovering UBSan. Full log inspected, Metadata read only, zero artifacts, no apparent secret leakage. Earlier local PASS claim lacks independent execution evidence; this CI supersedes it. GitHub-first: perform feasible development, testing, audit and checkpoints in GitHub; local only when safe GitHub execution is unavailable. No hardware validation.
