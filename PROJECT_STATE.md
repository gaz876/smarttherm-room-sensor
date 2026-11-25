# Current state — 2026-11-25

ESP32-C3 + SHT40 room sensor prototype: codec and temperature-only correction implemented; host C11 PASS reported at a908824. No target build, physical validation or complete firmware. Local MQTT integration with original SmartTherm is planned; exact topics, broker and freshness must be verified. GitHub-first development.

---
## Historical records (superseded where conflicting)

# Project state

2026-11-24: requirements baseline in dev. Experimental restart; no firmware, target build or hardware validation yet. Main remains unchanged. GitHub is the primary development location.

SHT40 codec and synthetic tests added. Temperature-only correction; humidity factory calibration only. Tests and target build not yet executed.

2026-11-24: host C11 tests PASS (exit 0) for a908824; all three source/test Git blob hashes matched the repository. Strict warnings enabled. ESP-IDF target build and physical sensor validation remain pending. No deployment or production integration performed.
