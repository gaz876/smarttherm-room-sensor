# Independent audit gate — 2026-11-25

Complete remaining history/dependency/CI audit; confirm datasheet boundaries and invalid/stale handling, including restart semantics; only after audit closure proceed to ESP-IDF target and isolated local MQTT integration. Preserve temperature-only correction, 10 s default, offline operation and no heating actuation. No new functional work during audit.

---

# Current next steps — 2026-11-25

Verify ESP-IDF, C3 pinout and SHT40 datasheet; audit autonomous public CI; validate codec/freshness, target build, Wi-Fi provisioning, MQTT broker/client authorization and exact SmartTherm topics, web UI and local OTA design. Hardware and deployment pending. No private data or paid runners. GitHub-first; synchronize public journals to private after each checkpoint.

---
## Historical records (superseded where conflicting)

# Next steps

1. Verify ESP-IDF version, board pinout and SHT40 datasheet.
2. Audit pinned CI dependencies and create standalone build workflow with read-only permissions.
3. Implement and test sensor decoding and freshness policy.
4. Implement Wi-Fi provisioning and persistence, MQTT authorization, web UI and local OTA.
5. Verify target build, hardware and safe stale-data handling before deployment.
