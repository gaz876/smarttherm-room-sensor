#ifndef ROOM_WIFI_POLICY_H
#define ROOM_WIFI_POLICY_H
#include <stdbool.h>
#include <stdint.h>

typedef enum { ROOM_WIFI_PROFILE_MISSING, ROOM_WIFI_PROFILE_VALID, ROOM_WIFI_PROFILE_ERROR } room_wifi_profile;
typedef enum { ROOM_WIFI_OFF, ROOM_WIFI_SETUP, ROOM_WIFI_RETRY, ROOM_WIFI_CONNECTING, ROOM_WIFI_ONLINE } room_wifi_state;
typedef enum { ROOM_WIFI_NONE, ROOM_WIFI_START_SETUP, ROOM_WIFI_CONNECT, ROOM_WIFI_STOP } room_wifi_action;
typedef struct {
    room_wifi_state state;
    room_wifi_profile profile;
    uint64_t deadline;
    uint32_t retry_ms;
} room_wifi_policy;

/* A single task owns the policy. The adapter must stop/drain the previous
 * transport before acting on a transition, and discard stale SDK events. */
static inline uint64_t wifi_deadline(uint64_t now, uint32_t delay) {
    return now > UINT64_MAX-delay ? UINT64_MAX : now+delay;
}
static inline room_wifi_action wifi_setup(room_wifi_policy *p, uint64_t now, bool authorized) {
    if (!authorized) return ROOM_WIFI_NONE;
    p->state=ROOM_WIFI_SETUP;
    p->deadline=wifi_deadline(now,300000u);
    return ROOM_WIFI_START_SETUP;
}
static inline room_wifi_action wifi_boot(room_wifi_policy *p, room_wifi_profile profile, uint64_t now, bool setup_ready) {
    *p=(room_wifi_policy){.state=ROOM_WIFI_OFF,.profile=profile,.retry_ms=1000u};
    if (profile==ROOM_WIFI_PROFILE_MISSING) return wifi_setup(p,now,setup_ready);
    if (profile==ROOM_WIFI_PROFILE_VALID) { p->state=ROOM_WIFI_RETRY; p->deadline=now; }
    return ROOM_WIFI_NONE;
}
static inline room_wifi_action wifi_manual(room_wifi_policy *p,uint64_t now,bool setup_ready) {
    /* Caller supplies one debounced physical BOOT gesture, not raw GPIO level. */
    if (p->state==ROOM_WIFI_SETUP) return ROOM_WIFI_NONE;
    return wifi_setup(p,now,setup_ready);
}
static inline void wifi_failed(room_wifi_policy *p,uint64_t now) {
    if (p->state!=ROOM_WIFI_CONNECTING && p->state!=ROOM_WIFI_ONLINE) return;
    p->state=ROOM_WIFI_RETRY;
    p->deadline=wifi_deadline(now,p->retry_ms);
    if (p->retry_ms<60000u) p->retry_ms=p->retry_ms>30000u?60000u:p->retry_ms*2u;
}
static inline void wifi_got_ip(room_wifi_policy *p) {
    if (p->state!=ROOM_WIFI_CONNECTING) return;
    p->state=ROOM_WIFI_ONLINE;
    p->retry_ms=1000u;
}
static inline room_wifi_action wifi_setup_done(room_wifi_policy *p,uint64_t now,bool persisted_and_verified) {
    if (p->state!=ROOM_WIFI_SETUP || !persisted_and_verified) return ROOM_WIFI_NONE;
    p->profile=ROOM_WIFI_PROFILE_VALID;
    p->state=ROOM_WIFI_RETRY;
    p->retry_ms=1000u;
    p->deadline=now;
    return ROOM_WIFI_STOP;
}
static inline room_wifi_action wifi_tick(room_wifi_policy *p,uint64_t now) {
    if (now<p->deadline) return ROOM_WIFI_NONE;
    if (p->state==ROOM_WIFI_SETUP) {
        p->state=p->profile==ROOM_WIFI_PROFILE_VALID?ROOM_WIFI_RETRY:ROOM_WIFI_OFF;
        p->deadline=now;
        p->retry_ms=1000u;
        return ROOM_WIFI_STOP;
    }
    if (p->state==ROOM_WIFI_CONNECTING) { wifi_failed(p,now); return ROOM_WIFI_STOP; }
    if (p->state==ROOM_WIFI_RETRY) {
        p->state=ROOM_WIFI_CONNECTING;
        p->deadline=wifi_deadline(now,30000u);
        return ROOM_WIFI_CONNECT;
    }
    return ROOM_WIFI_NONE;
}
#endif
