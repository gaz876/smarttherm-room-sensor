#include "room_apsta_events.h"
#include "esp_wifi.h"
#include "freertos/task.h"
#include <string.h>

/* A setup window admits at most one continuous association. A disconnect,
 * duplicate/ambiguous event or AP stop invalidates evidence for the window.
 * Epochs are never supplied by requests and never reused within this boot.
 * Queued old vendor events after teardown remain a target qualification gate. */
static bool peer_event(room_apsta_events *s,int32_t id,const void *data) {
    if (id!=WIFI_EVENT_AP_START && id!=WIFI_EVENT_AP_STOP &&
        id!=WIFI_EVENT_AP_STACONNECTED && id!=WIFI_EVENT_AP_STADISCONNECTED) return false;
    taskENTER_CRITICAL(&s->peer_lock);
    if (atomic_load(&s->accepting) && !s->peer_invalid) {
        if (id==WIFI_EVENT_AP_START && !s->peer_started && !s->peer_seen) s->peer_started=true;
        else if (id==WIFI_EVENT_AP_STACONNECTED && s->peer_started && !s->peer_seen && data && s->peer_epoch<UINT64_MAX) {
            const wifi_event_ap_staconnected_t *e=data;
            const uint8_t zero[6]={0};
            if (e->aid && !e->is_mesh_child && memcmp(e->mac,zero,sizeof(zero))!=0 && !(e->mac[0]&1u)) {
                memcpy(s->peer_mac,e->mac,sizeof(s->peer_mac));
                ++s->peer_epoch;s->peer_seen=true;s->peer_live=true;
            } else s->peer_invalid=true;
        } else s->peer_invalid=true;
        if (s->peer_invalid) s->peer_live=false;
    }
    taskEXIT_CRITICAL(&s->peer_lock);
    return true;
}
static void capture(void *ctx,esp_event_base_t base,int32_t id,void *data) {
    room_apsta_events *s=ctx;
    if (!atomic_load(&s->accepting)) return;
    if (base==WIFI_EVENT && peer_event(s,id,data)) return;
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
    portMUX_INITIALIZE(&s->peer_lock);
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
    taskENTER_CRITICAL(&s->peer_lock);
    s->peer_ap=NULL;s->peer_started=false;s->peer_seen=false;
    s->peer_live=false;s->peer_invalid=false;
    memset(s->peer_mac,0,sizeof(s->peer_mac));
    atomic_store(&s->accepting,1u);
    taskEXIT_CRITICAL(&s->peer_lock);
    return true;
}
void room_apsta_events_end(room_apsta_events *s) {
    if (!s || !s->queue) return;
    taskENTER_CRITICAL(&s->peer_lock);
    atomic_store(&s->accepting,0u);
    s->peer_ap=NULL;s->peer_live=false;s->peer_invalid=true;
    taskEXIT_CRITICAL(&s->peer_lock);
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

bool room_apsta_events_peer_attach(room_apsta_events *s,const esp_netif_t *ap) {
    if (!s || !ap || !s->queue || !s->bound || s->failed) return false;
    taskENTER_CRITICAL(&s->peer_lock);
    bool ok=atomic_load(&s->accepting) && !atomic_load(&s->overflow) && !s->peer_invalid && !s->peer_ap;
    if (ok) s->peer_ap=ap;
    taskEXIT_CRITICAL(&s->peer_lock);
    return ok;
}
void room_apsta_events_peer_detach(room_apsta_events *s) {
    if (!s || !s->queue) return;
    taskENTER_CRITICAL(&s->peer_lock);
    s->peer_ap=NULL;s->peer_live=false;s->peer_invalid=true;
    taskEXIT_CRITICAL(&s->peer_lock);
}
bool room_apsta_events_peer_epoch(room_apsta_events *s,const esp_netif_t *ap,const uint8_t mac[6],uint64_t *out) {
    if (!out) return false;
    *out=0;
    /* Initialized before publication. No callback may outlive this object. */
    if (!s || !ap || !mac || !atomic_load(&s->accepting)) return false;
    taskENTER_CRITICAL(&s->peer_lock);
    bool ok=atomic_load(&s->accepting) && !atomic_load(&s->overflow) &&
        s->peer_ap==ap && s->peer_live && !s->peer_invalid && s->peer_epoch &&
        memcmp(s->peer_mac,mac,sizeof(s->peer_mac))==0;
    if (ok) *out=s->peer_epoch;
    taskEXIT_CRITICAL(&s->peer_lock);
    return ok;
}

bool room_apsta_events_peer_fault(room_apsta_events *s) {
    if (!s || !atomic_load(&s->accepting)) return true;
    taskENTER_CRITICAL(&s->peer_lock);
    bool fault=!atomic_load(&s->accepting) || atomic_load(&s->overflow) || s->peer_invalid;
    taskEXIT_CRITICAL(&s->peer_lock);
    return fault;
}
