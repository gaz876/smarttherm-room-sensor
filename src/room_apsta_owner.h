#ifndef ROOM_APSTA_OWNER_H
#define ROOM_APSTA_OWNER_H
#include "room_apsta_prepare.h"
#include "esp_netif.h"
#include "esp_wifi_default.h"

/* Experimental, not called by firmware startup. One exclusive network task;
 * default event loop/netif subsystem already initialized. No other Wi-Fi user.
 * Caller proves physical window/identity readiness, owns transport and posts
 * actual event-loop barriers. One trial per fresh pair of netifs. */
typedef enum { APSTA_IDLE, APSTA_RUNNING, APSTA_STOP, APSTA_DRAIN,
    APSTA_FINAL_DRAIN, APSTA_LOCKED } room_apsta_phase;
typedef struct {
    room_apsta_phase phase;
    room_apsta_prepare_state config;
    esp_netif_t *ap, *sta;
    uint64_t window, last_window, trial;
    bool driver, attempted, ap_attached, sta_attached, uncertain;
} room_apsta_owner;
typedef bool (*room_apsta_barrier)(void *,uint64_t,bool);
static inline bool room_apsta_failed(room_apsta_owner *o,bool uncertain) {
    o->uncertain |= uncertain; o->phase=APSTA_STOP; return false;
}
static inline bool room_apsta_start(room_apsta_owner *o,uint64_t window,bool ready,
    const uint8_t *ssid,size_t n,const uint8_t *key,size_t m) {
    if (!o || o->phase!=APSTA_IDLE || !ready || !window || window<=o->last_window ||
        m<16 || !enroll_profile_valid(ssid,n,key,m)) return false;
    o->window=o->last_window=window; o->phase=APSTA_STOP;
    wifi_init_config_t init=WIFI_INIT_CONFIG_DEFAULT(); init.nvs_enable=0;
    if (esp_wifi_init(&init)!=ESP_OK) return room_apsta_failed(o,true);
    o->driver=true;
    esp_netif_config_t ac=ESP_NETIF_DEFAULT_WIFI_AP(), sc=ESP_NETIF_DEFAULT_WIFI_STA();
    o->ap=esp_netif_new(&ac);
    if (!o->ap) return false;
    if (esp_netif_attach_wifi_ap(o->ap)!=ESP_OK) return room_apsta_failed(o,true);
    o->ap_attached=true;
    o->sta=esp_netif_new(&sc);
    if (!o->sta) return false;
    if (esp_netif_attach_wifi_station(o->sta)!=ESP_OK) return room_apsta_failed(o,true);
    o->sta_attached=true;
    /* SDK registers shared AP and STA handlers in this call. */
    if (esp_wifi_set_default_wifi_ap_handlers()!=ESP_OK) return room_apsta_failed(o,true);
    if (!room_apsta_prepare(&o->config,ssid,n,key,m)) return room_apsta_failed(o,false);
    o->attempted=true;
    if (esp_wifi_start()!=ESP_OK) return room_apsta_failed(o,false);
    o->phase=APSTA_RUNNING; return true;
}
static inline bool room_apsta_ready(const room_apsta_owner *o,uint64_t window) {
    return o && o->phase==APSTA_RUNNING && window && window==o->window &&
        o->ap && esp_netif_is_netif_up(o->ap);
}
/* No implicit association. Require fresh STA with no cached address before
 * starting the one trial. The bridge must authorize the current candidate. */
static inline bool room_apsta_trial(room_apsta_owner *o,uint64_t window,uint64_t trial,
    const room_enroll_profile *p) {
    if (!room_apsta_ready(o,window) || !trial || o->trial || !p ||
        !enroll_profile_valid(p->ssid,p->ssid_len,p->pass,p->pass_len)) return false;
    esp_netif_ip_info_t ip={0};
    if (esp_netif_is_netif_up(o->sta) || esp_netif_get_ip_info(o->sta,&ip)!=ESP_OK || ip.ip.addr)
        return room_apsta_failed(o,false);
    o->trial=trial;
    if (!room_apsta_configure_trial(&o->config,p) || esp_wifi_connect()!=ESP_OK)
        return room_apsta_failed(o,false);
    return true;
}
/* Live observation is necessary but not sufficient to prove fresh DHCP.
 * Caller independently checks the lease/event for this exact fresh netif. */
static inline bool room_apsta_link(const room_apsta_owner *o,uint64_t window,uint64_t trial) {
    if (!room_apsta_ready(o,window) || !trial || trial!=o->trial) return false;
    wifi_ap_record_t ap; esp_netif_ip_info_t ip;
    return esp_wifi_sta_get_ap_info(&ap)==ESP_OK && esp_netif_is_netif_up(o->sta) &&
        esp_netif_get_ip_info(o->sta,&ip)==ESP_OK && ip.ip.addr!=0;
}
/* Caller has finished sending/closed and drained all transport I/O first.
 * Barrier must acknowledge separate pre-detach/post-deinit event phases.
 * A false return may mean pending or permanent lockout; inspect phase.
 * Never retry ambiguous SDK detach/stop/deinit. Owner must enforce outer
 * deadline and require controlled recovery for a permanently locked radio. */
typedef bool (*room_apsta_guard)(void *);
static inline bool room_apsta_guard_ok(room_apsta_owner *o,room_apsta_guard guard,void *ctx) {
    if (!guard || guard(ctx)) return true;
    o->phase=APSTA_LOCKED; return false;
}
static inline bool room_apsta_stop_guarded(room_apsta_owner *o,uint64_t window,bool transport_drained,
    room_apsta_barrier barrier,void *ctx,room_apsta_guard guard,void *guard_ctx) {
    if (!o || !window || window!=o->window || o->phase==APSTA_IDLE || o->phase==APSTA_LOCKED) return false;
    if (!room_apsta_guard_ok(o,guard,guard_ctx)) return false;
    if (o->phase==APSTA_RUNNING) o->phase=APSTA_STOP;
    if (!transport_drained || !barrier) return false;
    if (o->phase==APSTA_STOP) {
        if (o->attempted && esp_wifi_stop()!=ESP_OK) { o->phase=APSTA_LOCKED; return false; }
        o->attempted=false;
        if (o->uncertain) { o->phase=APSTA_LOCKED; return false; }
        o->phase=APSTA_DRAIN;
        if (!room_apsta_guard_ok(o,guard,guard_ctx)) return false;
    }
    if (o->phase==APSTA_DRAIN) {
        bool ready=barrier(ctx,window,false);
        if (!room_apsta_guard_ok(o,guard,guard_ctx) || !ready) return false;
        if ((o->ap && esp_netif_is_netif_up(o->ap)) || (o->sta && esp_netif_is_netif_up(o->sta))) return false;
        if (!room_apsta_guard_ok(o,guard,guard_ctx)) return false;
        if (o->sta) {
            if (o->sta_attached && esp_wifi_clear_default_wifi_driver_and_handlers(o->sta)!=ESP_OK) { o->phase=APSTA_LOCKED; return false; }
            if (!room_apsta_guard_ok(o,guard,guard_ctx)) return false;
            esp_netif_destroy(o->sta); o->sta=NULL;
            if (!room_apsta_guard_ok(o,guard,guard_ctx)) return false;
        }
        if (o->ap) {
            if (o->ap_attached && esp_wifi_clear_default_wifi_driver_and_handlers(o->ap)!=ESP_OK) { o->phase=APSTA_LOCKED; return false; }
            if (!room_apsta_guard_ok(o,guard,guard_ctx)) return false;
            esp_netif_destroy(o->ap); o->ap=NULL;
            if (!room_apsta_guard_ok(o,guard,guard_ctx)) return false;
        }
        if (o->driver && esp_wifi_deinit()!=ESP_OK) { o->phase=APSTA_LOCKED; return false; }
        o->driver=false; o->phase=APSTA_FINAL_DRAIN;
        if (!room_apsta_guard_ok(o,guard,guard_ctx)) return false;
    }
    bool ready=barrier(ctx,window,true);
    if (!room_apsta_guard_ok(o,guard,guard_ctx) || !ready) return false;
    uint64_t last=o->last_window;
    *o=(room_apsta_owner){.last_window=last}; return true;
}
static inline bool room_apsta_stop(room_apsta_owner *o,uint64_t window,bool transport_drained,
    room_apsta_barrier barrier,void *ctx) {
    return room_apsta_stop_guarded(o,window,transport_drained,barrier,ctx,NULL,NULL);
}
#endif
