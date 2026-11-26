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
    atomic_uint overflow;
    bool bound, failed;
} room_apsta_events;
bool room_apsta_events_bind(room_apsta_events *, QueueHandle_t);
bool room_apsta_events_poll(room_apsta_events *, room_apsta_session *);
/* Teardown only: discard queued records without clearing the overflow latch. */
bool room_apsta_events_discard(room_apsta_events *);
bool room_apsta_events_unbind(room_apsta_events *);
#endif
