#ifndef ROOM_HTTP_IDF_PEER_H
#define ROOM_HTTP_IDF_PEER_H
#include "room_http_idf_driver.h"

/* Qualification candidate. The AP owner must provide a continuous,
 * monotonically distinct association epoch for the currently live station.
 * The callback must be synchronized with the radio lifecycle, reject unknown
 * or ambiguous events, and never infer an epoch from request fields. No
 * fallback to IP/MAC identity alone. Owner/provider storage persists until
 * HTTP stop joins all callbacks. This is transport filtering, not auth. */
typedef bool (*room_http_ap_epoch)(void *,const esp_netif_t *,const uint8_t mac[6],uint64_t *);
typedef struct {
    void *ctx;
    room_http_ap_epoch epoch;
} room_http_idf_peer;
bool room_http_idf_peer_observe(void *,const esp_netif_t *,const httpd_req_t *,room_http_peer_observation *);
/* Explicit experimental construction; no call in the regular network task. */
static inline room_http_idf_peer room_http_idf_peer_local(void) {
    return (room_http_idf_peer){NULL,room_wifi_apsta_link_peer_epoch};
}
#endif
