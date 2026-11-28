#ifndef ROOM_APSTA_EVENTS_H
#define ROOM_APSTA_EVENTS_H
#include "room_apsta_session.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <stdatomic.h>

/* Single producer (default event loop), single consumer (network owner).
 * Dedicated queue of room_apsta_event records, created by owner before bind.
 * Context and queue must outlive unregister and a confirmed loop barrier. */
typedef struct {
    const esp_netif_t *netif;
    uint32_t ip;
    bool lost;
} room_apsta_event;
typedef struct {
    QueueHandle_t queue;
    esp_event_handler_instance_t ip_handler, wifi_handler;
    atomic_uint overflow, accepting;
    bool bound, failed;
    /* Shared by the owner, default event task and HTTP task. */
    portMUX_TYPE peer_lock;
    const esp_netif_t *peer_ap;
    uint64_t peer_epoch;
    uint8_t peer_mac[6];
    bool peer_started, peer_seen, peer_live, peer_invalid;
} room_apsta_events;
bool room_apsta_events_bind(room_apsta_events *, QueueHandle_t);
/* Capture is disabled after bind and between APSTA windows. Only the owner
 * may enable it, after confirmed STA cleanup, before opening APSTA. Disable
 * only after confirmed APSTA teardown or a rejected open with no window.
 * The gate does not identify SDK events delayed beyond the loop barrier. */
bool room_apsta_events_begin(room_apsta_events *);
void room_apsta_events_end(room_apsta_events *);
bool room_apsta_events_poll(room_apsta_events *, room_apsta_session *);
/* After a teardown marker, or before begin while capture is disabled.
 * Bounded discard never applies an event or clears the sticky overflow latch.
 * A poisoned event source rejects subsequent sessions until reboot. */
bool room_apsta_events_discard(room_apsta_events *);
bool room_apsta_events_unbind(room_apsta_events *);
/* Owner attaches the AP only after successful radio setup. This is local
 * association evidence, not user authentication or proof of SDK ordering. */
bool room_apsta_events_peer_attach(room_apsta_events *,const esp_netif_t *);
void room_apsta_events_peer_detach(room_apsta_events *);
bool room_apsta_events_peer_epoch(room_apsta_events *,const esp_netif_t *,const uint8_t [6],uint64_t *);
bool room_apsta_events_peer_fault(room_apsta_events *);
#endif
