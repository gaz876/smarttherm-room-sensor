# Experimental measurement settings

NVS namespace room_measure, key settings, version 1, a single 12-byte little-endian record (RS, version, reserved, interval in milliseconds, correction in millidegrees C biased by 10000). Range: 1000–3600000 ms, -10000–10000 mdeg. Default 10 s / 0 C (interval from Kconfig). Humidity unchanged.

Startup loads read-only. Missing/invalid/unsupported/I/O status is explicit; fallback defaults are not written automatically. No partition erase on NVS initialization failure. Save/reset helpers are single-owner operations for a future authenticated configuration path; they are not invoked at boot or per sample and there is no UI or runtime settings mutation yet. No-op saves do not write. Explicit reset can replace unsupported data, never erases other keys. On write/commit failure report failure; actual persisted state must be read back before future application. Hardware power-loss and real NVS testing remain required.

I2C recovery reapplies the loaded interval and correction. The measurement owner publishes a copy under a short FreeRTOS critical section; readers call room_runtime_snapshot in task context, with freshness calculated at read time and settings status available. No I2C or NVS calls under the lock. Future transport must check both sample validity and configuration status; invalid/unsupported/I/O configuration fallback is not cleared for heating use.

Host tests use a fake store, not real flash. New target build and hardware validation pending. No Wi-Fi, MQTT, OTA, heating or production changes.
