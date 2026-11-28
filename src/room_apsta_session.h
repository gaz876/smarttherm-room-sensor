#ifndef ROOM_APSTA_SESSION_H
#define ROOM_APSTA_SESSION_H
#include "room_apsta_owner.h"
#include "room_sec2_transport.h"

/* Uncalled experimental coordinator; only a single exclusive network task
 * may use it. No socket, task startup or identity provisioning. Event source
 * must be drained before trial and after radio teardown. */
typedef struct {
    room_apsta_owner radio;
    room_setup_bridge bridge;
    room_sec2_transport transport;
    room_setup_clock clock;
    void *ctx;
    uint32_t lease;
    uint64_t cleanup_deadline, cleanup_last;
    bool locked;
} room_apsta_session;
/* Owner supplies only queued SDK events, not client data. Lost events or
 * overflow invalidate the entire attempt. Never dereference event netif. */
void room_apsta_session_ip(room_apsta_session *, const esp_netif_t *, uint32_t);
void room_apsta_session_lost(room_apsta_session *);
bool room_apsta_session_open(room_apsta_session *, room_setup_clock, void *,
    phone_profile_state, bool, bool, const room_enroll_store *,
    const uint8_t *, size_t, const uint8_t *, size_t,
    const protocomm_security2_params_t *, uint32_t);
/* Call after processing pending SDK events in the owner task. Before first
 * trial, caller must prove its event-loop marker was acknowledged and its
 * event queue is drained; before each packet,
 * process all queued events. Actual sender owns pending buffer until finish. */
bool room_apsta_session_step(room_apsta_session *, bool events_drained);
bool room_apsta_session_close(room_apsta_session *, bool, room_apsta_barrier, void *);
/* Owner supplies the unchanged outer absolute deadline. */
bool room_apsta_session_close_until(room_apsta_session *, bool, room_apsta_barrier, void *, uint64_t);
#endif
