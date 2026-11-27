#include "room_apsta_events.h"

static void capture(void *ctx,esp_event_base_t base,int32_t id,void *data) {
    room_apsta_events *s=ctx;
    if (!atomic_load(&s->accepting)) return;
    room_apsta_event e={0};
    if (base==IP_EVENT && id==IP_EVENT_STA_GOT_IP) {
        if (!data) { atomic_store(&s->overflow,1u); return; }
        const ip_event_got_ip_t *got=data;
        e.netif=got->esp_netif; e.ip=got->ip_info.ip.addr;
    } else if ((base==IP_EVENT && id==IP_EVENT_STA_LOST_IP) ||
               (base==WIFI_EVENT && id==WIFI_EVENT_STA_DISCONNECTED)) e.lost=true;
    else return;
    if (xQueueSend(s->queue,&e,0)!=pdPASS) atomic_store(&s->overflow,1u);
}
bool room_apsta_events_bind(room_apsta_events *s,QueueHandle_t queue) {
    if (!s || !queue || s->queue || s->failed) return false;
    s->queue=queue; atomic_init(&s->overflow,0u); atomic_init(&s->accepting,0u);
    if (esp_event_handler_instance_register(IP_EVENT,ESP_EVENT_ANY_ID,capture,s,&s->ip_handler)!=ESP_OK ||
        esp_event_handler_instance_register(WIFI_EVENT,ESP_EVENT_ANY_ID,capture,s,&s->wifi_handler)!=ESP_OK) {
        s->failed=true; return false;
    }
    s->bound=true; return true;
}
bool room_apsta_events_discard(room_apsta_events *s) {
    if (!s || !s->bound || s->failed) return false;
    room_apsta_event e;
    for (unsigned i=0;i<8;i++) if (xQueueReceive(s->queue,&e,0)!=pdPASS) break;
    return uxQueueMessagesWaiting(s->queue)==0;
}
/* The exclusive owner must have confirmed the previous radio's teardown
 * before begin. Persistent callback storage is never released here. */
bool room_apsta_events_begin(room_apsta_events *s) {
    if (!s || !s->bound || s->failed || atomic_load(&s->accepting) ||
        atomic_load(&s->overflow) || !room_apsta_events_discard(s) ||
        atomic_load(&s->overflow)) return false;
    atomic_store(&s->accepting,1u);
    return true;
}
void room_apsta_events_end(room_apsta_events *s) {
    if (s && s->queue) atomic_store(&s->accepting,0u);
}
bool room_apsta_events_poll(room_apsta_events *s,room_apsta_session *session) {
    if (!s || !session || !s->bound || s->failed) return false;
    if (atomic_load(&s->overflow)) { room_apsta_session_lost(session); return false; }
    room_apsta_event e;
    for (unsigned i=0;i<8;i++) {
        if (xQueueReceive(s->queue,&e,0)!=pdPASS) break;
        if (e.lost) room_apsta_session_lost(session);
        else room_apsta_session_ip(session,e.netif,e.ip);
    }
    if (atomic_load(&s->overflow)) { room_apsta_session_lost(session); return false; }
    return uxQueueMessagesWaiting(s->queue)==0;
}
bool room_apsta_events_unbind(room_apsta_events *s) {
    if (!s || !s->queue) return false;
    room_apsta_events_end(s);
    s->bound=false;
    if (s->ip_handler) {
        if (esp_event_handler_instance_unregister(IP_EVENT,ESP_EVENT_ANY_ID,s->ip_handler)!=ESP_OK) { s->failed=true; return false; }
        s->ip_handler=NULL;
    }
    if (s->wifi_handler) {
        if (esp_event_handler_instance_unregister(WIFI_EVENT,ESP_EVENT_ANY_ID,s->wifi_handler)!=ESP_OK) { s->failed=true; return false; }
        s->wifi_handler=NULL;
    }
    /* Caller still needs an event-loop barrier before releasing storage.
     * Instance is single-use, queue pointer retained intentionally. */
    return !s->failed;
}
