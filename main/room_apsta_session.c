#include "room_apsta_session.h"

static uint64_t session_clock(void *v) {
    room_apsta_session *s=v; return s->clock(s->ctx);
}
static bool session_live(void *v,uint64_t trial) {
    room_apsta_session *s=v;
    esp_netif_ip_info_t ip={0};
    esp_netif_dhcp_status_t dhcp=ESP_NETIF_DHCP_INIT;
    return !s->locked && s->lease && room_apsta_link(&s->radio,s->bridge.window,trial) &&
        esp_netif_dhcpc_get_status(s->radio.sta,&dhcp)==ESP_OK && dhcp==ESP_NETIF_DHCP_STARTED &&
        esp_netif_get_ip_info(s->radio.sta,&ip)==ESP_OK && ip.ip.addr==s->lease;
}
void room_apsta_session_ip(room_apsta_session *s,const esp_netif_t *netif,uint32_t ip) {
    if (!s || s->locked || s->bridge.stopping || !s->radio.trial || !netif ||
        netif!=s->radio.sta || !ip || s->radio.phase!=APSTA_RUNNING) return;
    s->lease=ip;
}
void room_apsta_session_lost(room_apsta_session *s) {
    if (!s || !s->bridge.window) return;
    s->lease=0; bridge_stop(&s->bridge);
}
bool room_apsta_session_open(room_apsta_session *s,room_setup_clock clock,void *ctx,
    phone_profile_state profile,bool boot,bool trusted,const room_enroll_store *store,
    const uint8_t *ssid,size_t n,const uint8_t *key,size_t m,
    const protocomm_security2_params_t *id,uint32_t sdk_id) {
    if (!s || s->locked || s->bridge.window || s->radio.phase!=APSTA_IDLE ||
        s->transport.bridge || !clock || !trusted) return false;
    s->clock=clock; s->ctx=ctx;
    uint64_t w=bridge_open(&s->bridge,clock(ctx),profile,boot,trusted,store);
    if (!w) return false;
    /* Allocate security context before starting AP. No network exposure here. */
    if (!room_sec2_transport_start(&s->transport,&s->bridge,id,sdk_id,w,store,
        session_clock,session_live,s) || !room_apsta_start(&s->radio,w,trusted,ssid,n,key,m)) {
        bridge_stop(&s->bridge); return false;
    }
    return true;
}
bool room_apsta_session_step(room_apsta_session *s,bool events_drained) {
    if (!s || s->locked || !s->bridge.window) return false;
    uint64_t now=session_clock(s); bridge_tick(&s->bridge,now);
    if (s->bridge.stopping || s->transport.stopped) return false;
    if (s->transport.pending && now>=s->transport.deadline) {
        /* Keep the borrowed reply alive until the actual sender drains. */
        bridge_stop(&s->bridge); return false;
    }
    if (s->bridge.life.phase==SETUP_FINAL || s->transport.pending) return true;
    if (s->bridge.phone.drain || s->radio.phase!=APSTA_RUNNING) {
        bridge_stop(&s->bridge); return false;
    }
    room_enroll *e=&s->bridge.phone.enrollment;
    if (e->state==ENROLL_IDLE || !events_drained) return true;
    if (!s->radio.trial) {
        if (!room_apsta_ready(&s->radio,s->bridge.window)) return true;
        if (!room_apsta_trial(&s->radio,s->bridge.window,e->generation,&e->candidate)) {
            bridge_stop(&s->bridge); return false;
        }
    }
    if (s->radio.trial!=e->generation) { bridge_stop(&s->bridge); return false; }
    bool live=session_live(s,e->generation);
    if (e->state==ENROLL_VERIFIED && !live) { bridge_stop(&s->bridge); return false; }
    if (e->state==ENROLL_TRIAL && live) (void)bridge_trial(&s->bridge,now,s->bridge.window,e->generation,true);
    return !s->bridge.stopping;
}
bool room_apsta_session_close(room_apsta_session *s,bool sender_drained,room_apsta_barrier barrier,void *ctx) {
    if (!s || s->locked || !s->bridge.window) return false;
    uint64_t now=session_clock(s); bridge_stop(&s->bridge); s->lease=0;
    if (!s->cleanup_deadline) s->cleanup_deadline=now>UINT64_MAX-5000u?UINT64_MAX:now+5000u;
    if (now>=s->cleanup_deadline) { s->locked=true; return false; }
    if (!sender_drained || !barrier) return false;
    uint64_t w=s->bridge.window;
    if (s->radio.phase!=APSTA_IDLE && !room_apsta_stop(&s->radio,w,true,barrier,ctx)) {
        if (s->radio.phase==APSTA_LOCKED) s->locked=true;
        return false;
    }
    /* Only now may the security adapter acknowledge logical window drain. */
    bool ok=s->transport.bridge?room_sec2_transport_destroy(&s->transport,true):bridge_drained(&s->bridge,now,w);
    if (!ok) { s->locked=true; return false; }
    s->cleanup_deadline=0; return true;
}
