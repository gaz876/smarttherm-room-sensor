#ifndef ROOM_SEC2_TRANSPORT_H
#define ROOM_SEC2_TRANSPORT_H
#include "room_sec2_context.h"
#include "room_setup_bridge.h"
/* Experimental offline packet adapter. Never expose to a live network until
 * the SDK decrypted-buffer lifecycle and trusted identity are approved.
 * Single owner; all pointers are borrowed until destroy, except reply owned
 * by this adapter. Caller must stop/drain actual I/O before destroy. */
typedef struct {
    room_sec2_context sec;
    room_setup_bridge *bridge;
    const room_enroll_store *store;
    room_setup_clock clock;
    room_setup_live live;
    void *ctx;
    uint64_t window, generation, ticket, deadline;
    uint8_t *reply;
    size_t reply_len;
    bool dispatching, claimed, pending, stopped;
} room_sec2_transport;
bool room_sec2_transport_start(room_sec2_transport *, room_setup_bridge *,
    const protocomm_security2_params_t *, uint32_t, uint64_t,
    const room_enroll_store *, room_setup_clock, room_setup_live, void *);
bool room_sec2_transport_exchange(room_sec2_transport *, bool handshake,
    const uint8_t *, size_t, const uint8_t **, size_t *);
bool room_sec2_transport_finish(room_sec2_transport *, bool sent);
bool room_sec2_transport_destroy(room_sec2_transport *, bool drained);
#endif
