# HTTPS isolation audit (2026-11-26)

## Exact SDK evidence

Current build is ESP-IDF **5.5.6**, source [880a203](https://github.com/espressif/esp-idf/tree/880a203ae603d08a94d06aa01e0d6761958e5e9e) verified by tools/cmake/version.cmake. The checked source, not a newer online manual, determines available APIs.

- [HTTP server configuration](https://github.com/espressif/esp-idf/blob/880a203ae603d08a94d06aa01e0d6761958e5e9e/components/esp_http_server/include/esp_http_server.h): no interface-name member.
- [HTTP server listener and stop](https://github.com/espressif/esp-idf/blob/880a203ae603d08a94d06aa01e0d6761958e5e9e/components/esp_http_server/src/httpd_main.c#L363-L393): wildcard bind. [Stop](https://github.com/espressif/esp-idf/blob/880a203ae603d08a94d06aa01e0d6761958e5e9e/components/esp_http_server/src/httpd_main.c#L556-L585) waits for server thread shutdown without overall deadline.
- [HTTPS implementation](https://github.com/espressif/esp-idf/blob/880a203ae603d08a94d06aa01e0d6761958e5e9e/components/esp_https_server/src/https_server.c): calls httpd_start and httpd_stop. Software private key bytes copied into SDK heap at lines 352–357 and released by free at 254–255; this particular buffer is not explicitly wiped. Other TLS-layer copies were not audited in this pass. Do not promise complete memory zeroization.

A URI-level filter is not equivalent to binding the listening socket to AP. Do not enable this server on current APSTA build.

Official **6.1.1** tag resolves to commit [051e4c4](https://github.com/espressif/esp-idf/tree/051e4c4eb20346e043cdd47bc388fde75ff8af63).

- [HTTP header](https://github.com/espressif/esp-idf/blob/051e4c4eb20346e043cdd47bc388fde75ff8af63/components/esp_http_server/include/esp_http_server.h#L244-L247): interface name field.
- [HTTP implementation](https://github.com/espressif/esp-idf/blob/051e4c4eb20346e043cdd47bc388fde75ff8af63/components/esp_http_server/src/httpd_main.c#L393-L413): interface binding attempted before bind; returns failure rather than falling back on failed interface binding. Empty or null interface name is still unsafe for this use case.
- [HTTPS header](https://github.com/espressif/esp-idf/blob/051e4c4eb20346e043cdd47bc388fde75ff8af63/components/esp_https_server/include/esp_https_server.h): HTTPS config includes interface-name field and handshake timeout.
- [Netif header](https://github.com/espressif/esp-idf/blob/051e4c4eb20346e043cdd47bc388fde75ff8af63/components/esp_netif/include/esp_netif.h#L491): API for interface implementation name.
- Shutdown still blocking in v6.1.1. Interface bind is a capability, not runtime proof of AP isolation.

## Selected next experiment

Qualify official v6.1.1 in an isolated build, retain v5.5.6 baseline. No custom HTTP parser, no wildcard fallback, no dependency upgrade in active branch. Docker image digest for exact v6.1.1 not independently verified in this pass; do not run an unpinned tag. Record verified image digest, SDK commit, compiler, submodules and configuration, run existing 51 scenarios and target build; compare flash and RAM. Follow official [migration guide](https://docs.espressif.com/projects/esp-idf/en/v6.1.1/esp32c3/migration-guides/index.html).

The standalone tests/https_sdk_probe project is compile-only. It requires the interface-bind member and HTTPS APIs and has an empty app_main. It is never included in production. Expected to fail to compile on the old SDK; no actual compile attempted in this pass. It is not a server and must not be flashed as operational firmware.

Server ownership: dedicated worker outside network owner, bounded copied requests/replies, no SDK request pointer passed to Wi-Fi owner, explicit worker-stop acknowledgement. Never call blocking httpd_ssl_stop in owner task. AP-only ingress, actual isolation through AP/STA trials, certificate trust, credential provisioning, memory and shutdown must be demonstrated on hardware before activation. Browser Security2 still needs a reviewed client implementation and interoperability evidence.
