#include "room_wifi_apsta_link.h"
#include "room_wifi_secure_store.h"
#include "esp_timer.h"

/* Lifetime is the boot; the persistent queue and registered callbacks must
 * never reference a freed or moved context. This code is linked but uncalled
 * by normal startup. Only the station owner's task can bind it. */
static struct {
    room_apsta_task task;
    room_enroll_store store;
    room_wifi_apsta_provider provider;
    bool attempted;
} a;
static uint64_t now_ms(void) { return (uint64_t)(esp_timer_get_time()/1000); }
static bool bind(void *ctx) {
    (void)ctx; a.store=room_secure_store();
    return room_apsta_task_bind(&a.task);
}
static bool ready(void *ctx) {
    (void)ctx;
    return a.task.ready && !a.task.failed && !a.task.session.locked &&
        !atomic_load(&a.task.events.overflow) &&
        enroll_ready(&a.store) && a.provider.ready(a.provider.ctx);
}
static bool start(void *ctx,uint64_t deadline) {
    (void)ctx;
    if (now_ms()>=deadline || !ready(NULL)) return false;
    room_wifi_apsta_window p={0};
    bool ok=a.provider.prepare(a.provider.ctx,&p) && ready(NULL) &&
        now_ms()<deadline && p.key_len>=16 &&
        enroll_profile_valid(p.ssid,p.ssid_len,p.key,p.key_len) &&
        p.identity && p.identity->salt && p.identity->salt_len>=8 &&
        p.identity->salt_len<=64 && p.identity->verifier &&
        p.identity->verifier_len==384 && p.sdk_id && p.sdk_id!=UINT32_MAX &&
        room_apsta_task_open(&a.task,PHONE_PROFILE_VALID,true,true,&a.store,
            p.ssid,p.ssid_len,p.key,p.key_len,p.identity,p.sdk_id);
    enroll_wipe(&p,sizeof(p));
    if (ok && a.task.session.bridge.phone.deadline>deadline)
        a.task.session.bridge.phone.deadline=deadline;
    return ok;
}
static bool poll(void *ctx) { (void)ctx; return room_apsta_task_poll(&a.task); }
static bool stop(void *ctx) {
    (void)ctx;
    room_apsta_session *s=&a.task.session;
    if (s->locked || s->radio.phase==APSTA_LOCKED) return false;
    if (!s->bridge.window)
        return s->radio.phase==APSTA_IDLE && !s->transport.bridge &&
            !s->transport.pending && !a.task.pending &&
            !atomic_load(&a.task.events.accepting);
    /* There is no listener or sender in this candidate. Never claim drain if
     * a reply or dispatch is outstanding; a future transport must prove it. */
    bool drained=!s->transport.pending && !s->transport.dispatching;
    return room_apsta_task_close(&a.task,drained);
}
static bool load(void *ctx,room_enroll_profile *p) {
    (void)ctx; return room_secure_load(p)==ROOM_SEC_VALID;
}
bool room_wifi_apsta_link_bind(room_wifi_apsta_provider provider) {
    if (!room_wifi_idf_is_owner() || a.attempted || !provider.ready || !provider.prepare) return false;
    a.attempted=true; a.provider=provider;
    return room_wifi_idf_bind_setup((room_wifi_setup_io){NULL,bind,ready,start,poll,stop,load});
}
