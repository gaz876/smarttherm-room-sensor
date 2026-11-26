#ifndef ROOM_WIFI_OWNER_H
#define ROOM_WIFI_OWNER_H
#include "room_wifi_policy.h"

typedef enum { WIFI_EVENT_NONE, WIFI_EVENT_IP, WIFI_EVENT_LOST, WIFI_EVENT_SETUP_SAVED } room_wifi_event_kind;
typedef struct { room_wifi_event_kind kind; uint64_t generation; } room_wifi_event;
typedef struct {
    void *ctx;
    /* True only after old transport is stopped and its event source is quiescent.
     * False means cleanup pending; caller retries. A failed start may need cleanup. */
    bool (*stop)(void *);
    bool (*station)(void *,uint64_t);
    bool (*setup)(void *,uint64_t);
} room_wifi_transport;
typedef struct {
    room_wifi_policy policy;
    room_wifi_transport io;
    uint64_t generation, active, cleanup_at;
    bool dirty, exhausted, pending_setup;
} room_wifi_owner;
static inline bool wifi_owner_stop(room_wifi_owner *o,uint64_t now) {
    o->active=0;
    if (!o->dirty) return true;
    if (o->io.stop && o->io.stop(o->io.ctx)) { o->dirty=false; return true; }
    o->cleanup_at=wifi_deadline(now,1000u);
    return false;
}
static inline void wifi_owner_abort_start(room_wifi_owner *o,uint64_t now) {
    if (o->policy.state==ROOM_WIFI_CONNECTING) wifi_failed(&o->policy,now);
    else if (o->policy.state==ROOM_WIFI_SETUP) {
        o->policy.state=o->policy.profile==ROOM_WIFI_PROFILE_VALID?ROOM_WIFI_RETRY:ROOM_WIFI_OFF;
        o->policy.deadline=wifi_deadline(now,1000u);
    }
}
static inline void wifi_owner_action(room_wifi_owner *o,room_wifi_action action,uint64_t now) {
    if (action==ROOM_WIFI_NONE) return;
    if (!wifi_owner_stop(o,now)) {
        if (action==ROOM_WIFI_START_SETUP) o->pending_setup=true;
        else wifi_owner_abort_start(o,now);
        return;
    }
    if (action==ROOM_WIFI_STOP) return;
    if (o->generation==UINT64_MAX) {
        o->exhausted=true; o->policy.state=ROOM_WIFI_OFF; return;
    }
    uint64_t token=++o->generation;
    o->dirty=true;
    bool ok=action==ROOM_WIFI_CONNECT ? (o->io.station && o->io.station(o->io.ctx,token))
                                      : (o->io.setup && o->io.setup(o->io.ctx,token));
    if (ok) o->active=token;
    else { wifi_owner_abort_start(o,now); (void)wifi_owner_stop(o,now); }
}
/* Initialization once, after hardware transport setup; only one owner calls this API. */
static inline void wifi_owner_init(room_wifi_owner *o,room_wifi_transport io,room_wifi_profile profile,uint64_t now,bool setup_ready) {
    *o=(room_wifi_owner){.io=io};
    if (!io.stop || !io.station || !io.setup) { o->exhausted=true; return; }
    wifi_owner_action(o,wifi_boot(&o->policy,profile,now,setup_ready),now);
}
/* Call periodically, even without events. Deadline wins over late events.
 * Tokens must be bound to the event producer, never assigned on dequeue.
 * SETUP_SAVED is a trusted local result of authenticated, verified persistence. */
static inline void wifi_owner_step(room_wifi_owner *o,uint64_t now,bool manual,bool setup_ready,room_wifi_event e) {
    if (o->exhausted) return;
    /* Latch a one-shot authorized BOOT gesture even during asynchronous
     * cleanup. The original setup deadline is never extended by retries. */
    if (manual && setup_ready && now!=UINT64_MAX &&
        wifi_manual(&o->policy,now,true)==ROOM_WIFI_START_SETUP) o->pending_setup=true;
    if (o->pending_setup && (!setup_ready || now>=o->policy.deadline)) {
        o->pending_setup=false;
        wifi_owner_abort_start(o,now);
    }
    if (o->dirty && !o->active) {
        if (now<o->cleanup_at || !wifi_owner_stop(o,now)) return;
    }
    if (o->pending_setup) {
        o->pending_setup=false;
        wifi_owner_action(o,ROOM_WIFI_START_SETUP,now);
        return;
    }
    wifi_owner_action(o,wifi_tick(&o->policy,now),now);
    if (o->exhausted || (o->dirty && !o->active)) return;
    if (!o->active || e.generation!=o->active) return;
    if (e.kind==WIFI_EVENT_IP) wifi_got_ip(&o->policy);
    else if (e.kind==WIFI_EVENT_LOST && (o->policy.state==ROOM_WIFI_CONNECTING || o->policy.state==ROOM_WIFI_ONLINE)) {
        wifi_failed(&o->policy,now); (void)wifi_owner_stop(o,now);
    } else if (e.kind==WIFI_EVENT_SETUP_SAVED) {
        wifi_owner_action(o,wifi_setup_done(&o->policy,now,true),now);
    }
}
static inline bool wifi_owner_online(const room_wifi_owner *o) {
    return o->active!=0 && o->policy.state==ROOM_WIFI_ONLINE;
}

/* Active-low physical BOOT sampling; require stable release after startup.
 * One gesture per hold, 50 ms debounce, 3 s hold. Monotonic clock required. */
typedef struct { bool initialized,raw,stable,armed,fired; uint64_t changed,pressed; } room_boot_button;
static inline bool wifi_boot_sample(room_boot_button *b,bool down,uint64_t now) {
    if (!b->initialized) { *b=(room_boot_button){.initialized=true,.raw=down,.stable=true,.changed=now}; return false; }
    if (down!=b->raw) { b->raw=down; b->changed=now; }
    if (now-b->changed>=50u && b->stable!=b->raw) {
        b->stable=b->raw;
        if (!b->stable) { b->armed=true; b->fired=false; }
        else b->pressed=now;
    }
    if (b->armed && b->stable && b->raw && !b->fired && now-b->pressed>=3000u) {
        b->fired=true; return true;
    }
    return false;
}
#endif
