# SmartTherm room sensor

Experimental standalone ESP32-C3 / SHT40 firmware. Not ready for heating control.

## Requirements
- Temperature and humidity; configurable sampling (default 10 s), calibration, CRC validation and explicit invalid/stale state.
- Wi-Fi 2.4 GHz; protected initial phone setup, persistent configuration, indefinite reconnect with backoff, manual BOOT setup, no automatic AP on connection loss; DHCP or static IP.
- Local MQTT broker and external broker client modes; configurable topics, indoor/outdoor sources, multiple sensors and reconnect. Verify SmartTherm protocol compatibility before implementation.
- Russian responsive web UI: measurements, age, IP, RSSI, clients, sensor status, firmware, heap, uptime and reset reason; authenticated reboot and factory reset with confirmation.
- Individual web credentials, MQTT authentication and topic ACL, watchdog, no embedded secrets, no production dependencies. Fail closed on authentication errors.
- Local browser OTA with integrity and compatibility checks and rollback. No automatic GitHub OTA. No graphs.

## Engineering gates
Use ESP-IDF, verify board pinout and dependency versions. Proposed I2C GPIO4/5 and address 0x44 require hardware verification. Host tests and target builds must pass; hardware and stale-temperature behavior must be verified before use with heating equipment. No live boiler testing until safe fallback is verified.

Public CI must be standalone, use synthetic data, read-only token permissions, pinned third-party actions, and no private services or credentials. Audit before publishing artifacts.
