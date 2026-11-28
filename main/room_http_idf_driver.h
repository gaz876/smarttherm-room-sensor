#ifndef ROOM_HTTP_IDF_DRIVER_H
#define ROOM_HTTP_IDF_DRIVER_H
#include "room_http_backend.h"
#include "room_http_peer_guard.h"

/* Experimental SDK transport. A separately qualified, local AP peer policy
 * is mandatory. It must prove current association with the same single peer
 * across this window, and reject changes; a remote field is not evidence.
 * No default instance and no automatic startup. Context persists until boot. */
typedef bool (*room_http_peer_observe)(void *,const esp_netif_t *,const httpd_req_t *,room_http_peer_observation *);
typedef struct {
    void *peer_ctx;
    room_http_peer_observe observe;
    room_http_peer_guard guard;
    uint8_t scratch[ROOM_PHONE_PACKET_MAX];
    bool initialized;
} room_http_idf_driver;
bool room_http_idf_driver_init(room_http_idf_driver *,void *,room_http_peer_observe,room_http_driver *);
#endif
