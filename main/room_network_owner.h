#ifndef ROOM_NETWORK_OWNER_H
#define ROOM_NETWORK_OWNER_H
#include "room_wifi_apsta_link.h"
#include "room_wifi_secure_store.h"

/* Task-context only, once before the first Wi-Fi owner step. A NULL backend
 * preserves the station-only startup. The non-NULL path is experimental;
 * the caller must supply a qualified local timed backend with boot lifetime.
 * On failure the caller must not step or retry the partially initialized owner. */
typedef struct {
    room_phone_io io;
    room_phone_timed_close close_until;
} room_network_setup_backend;
bool room_network_owner_prepare(unsigned key_id,
    const room_network_setup_backend *backend, room_secure_status *status);
#endif
