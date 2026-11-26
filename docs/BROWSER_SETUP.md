# Browser setup: trust gate and UI preview

Status: design candidate, not an operational HTTP or HTTPS endpoint. The previous 51 host scenarios and ESP32-C3 build passed for 4616624dd00abe44f0b2dabce2b44b657c85383d in run 45784779902. No firmware code, startup, dependencies, keys, eFuse, or partitions changed in this commit.

## Decision

The Russian browser interface remains a requirement; no Android app, internet service or CDN dependency. A device-hosted page over ordinary HTTP cannot independently authenticate the code delivered to a browser. Web Crypto SubtleCrypto is secure-context-only; an ordinary private IP address over HTTP is not equivalent to localhost. Do not send router credentials through such a page, add plaintext fallback, or instruct users to ignore certificate errors.

Proposed route for review: ESP-IDF HTTPS with an individually authenticated device certificate (matching the actual AP URL/hostname or IP SAN) and trust anchor installed on the phone through a separate trusted local process. Test on the actual Android 10 phone/browser and other supported clients before approval. Certificate trust enrollment, DNS/IP naming, renewal, device key storage and revocation remain open. No certificates, key provisioning or real credentials in public source. This proposal retains Security 2 and existing authenticated save policy; HTTPS does not replace it without an explicit design decision. Browser Security 2 client requires a reviewed dependency and interoperability tests against the exact pinned ESP-IDF, not a new hand-written cryptographic protocol.

Alternative: independently distributed trusted client using the official Espressif protocol. That is not the preferred browser-only UX, and no switch to it is authorized. A self-signed warning click-through is not a trust solution.

## Implementation gates

1. Offline provisioning of trust anchor and individual device identity, browser validates correct certificate without warning.
2. HTTPS backend restricted to setup AP interface and current local peer, not STA or internet; no forwarding, no default HTTP credential endpoint.
3. Bounded, cancellable requests with one in-flight response; owner task only for Wi-Fi policy and protected storage. A backend must confirm shutdown and buffer drain, including partial open or interrupted responses.
4. Browser Security 2 interoperability against actual SDK (handshake, authenticated command, save receipt, rejected invalid credentials, expiry). No plaintext fallback or secrets in URLs, persistent browser storage, logs or telemetry. Use local bundled resources only.
5. Only enable input after independent transport trust and protocol authentication. Test phone browser, AP expiry, Wi-Fi loss, STA isolation, real DHCP and protected NVS before enabling startup.

## Preview

docs/setup-preview.html is inert, standalone, Russian, responsive HTML/CSS. No JavaScript, form submission, network requests, actual credentials or firmware embedding. Disabled input fields and buttons illustrate the future flow, not an implemented capability. The preview shows explicit trust/identity, DHCP, test, save and reconnect steps. Browser rendering and mobile device behavior still need testing.

## References

- MDN Web Crypto API: https://developer.mozilla.org/en-US/docs/Web/API/Web_Crypto_API
- MDN Secure contexts: https://developer.mozilla.org/en-US/docs/Web/Security/Defenses/Secure_Contexts
- Espressif protocomm: https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-reference/provisioning/protocomm.html
- Espressif unified provisioning: https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-reference/provisioning/provisioning.html

The documentation is a reference for the architecture, not proof of compatibility with the pinned SDK or actual browser.
