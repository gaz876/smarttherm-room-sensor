#ifndef ROOM_SETUP_BRIDGE_H
#define ROOM_SETUP_BRIDGE_H
#include "room_setup_dispatch.h"
#include "room_setup_lifecycle.h"

/* Single-owner offline integration model. No SDK calls. The caller owns AP
 * startup, authenticated identity, real trial I/O and all transport buffers.
 * All callbacks are synchronous, bounded and non-reentrant. The caller must
 * poll and supply a trusted monotonic clock, never a request timestamp. */
typedef uint64_t (*room_setup_clock)(void *);
typedef struct {
    room_phone_setup phone;
    room_setup_lifecycle life;
    room_setup_dispatch dispatch;
    uint64_t window, last_window;
    bool stopping;
} room_setup_bridge;
static inline void bridge_stop(room_setup_bridge *b) {
    if (!b->window) return;
    phone_close(&b->phone); setup_life_stop(&b->life); b->stopping=true;
}
static inline void bridge_tick(room_setup_bridge *b,uint64_t now) {
    setup_life_tick(&b->life,now);
    phone_tick(&b->phone,now);
    if (b->window && (b->life.failed || b->life.phase==SETUP_STOP ||
        (!b->phone.window && b->life.phase!=SETUP_FINAL))) bridge_stop(b);
}
/* Window identity is independent of authenticated session: even a window
 * that never authenticates requires confirmed AP cleanup before reopening. */
static inline uint64_t bridge_open(room_setup_bridge *b,uint64_t now,phone_profile_state profile,
    bool boot,bool identity_ready,const room_enroll_store *store) {
    bridge_tick(b,now);
    if (b->window || b->life.failed || b->last_window==UINT64_MAX || now==UINT64_MAX) return 0;
    if (!phone_open(&b->phone,now,profile,boot,identity_ready,store)) return 0;
    b->window=++b->last_window; return b->window;
}
static inline bool bridge_claim(room_setup_bridge *b,uint64_t now,uint64_t window,uint64_t session) {
    bridge_tick(b,now);
    if (!window || window!=b->window || b->stopping || b->life.phase!=SETUP_DOWN ||
        session<=b->life.last_session || !phone_claim(&b->phone,now,session)) return false;
    if (setup_life_open(&b->life,now,session)) return true;
    bridge_stop(b); return false;
}
/* Trial identity is independent of window identity. Cleanup callbacks must
 * carry the exact window and trial captured by the actual producer. */
static inline bool bridge_trial(room_setup_bridge *b,uint64_t now,uint64_t window,uint64_t trial,bool live) {
    bridge_tick(b,now);
    return window && window==b->window && !b->stopping &&
        b->life.phase==SETUP_OPEN && phone_trial_result(&b->phone,now,trial,live);
}
static inline bool bridge_trial_drained(room_setup_bridge *b,uint64_t now,uint64_t window,uint64_t trial) {
    bridge_tick(b,now);
    if (!window || window!=b->window || b->stopping || b->life.phase!=SETUP_OPEN ||
        !b->phone.drain || !trial || trial!=b->phone.enrollment.generation) return false;
    phone_drained(&b->phone); return true;
}
/* Guard a write before it begins, including time spent in readiness/link
 * callbacks. A write already started cannot be undone or preempted here;
 * always allow its readback and report no late reply. */
typedef struct {
    room_setup_bridge *b;
    const room_enroll_store *store;
    room_setup_clock clock;
    room_setup_live live;
    void *ctx;
    uint64_t seen;
    bool bad_clock;
} room_setup_guard;
static inline bool bridge_guard_time(room_setup_guard *g) {
    uint64_t now=g->clock(g->ctx);
    if (now<g->seen) g->bad_clock=true;
    if (now>g->seen) g->seen=now;
    return !g->bad_clock && now<g->b->life.deadline && now<g->b->phone.deadline &&
        (g->b->phone.enrollment.state==ENROLL_IDLE || now<g->b->phone.enrollment.deadline);
}
static inline bool bridge_ready(void *v) {
    room_setup_guard *g=v;
    return bridge_guard_time(g) && enroll_ready(g->store) && bridge_guard_time(g);
}
static inline bool bridge_write(void *v,const room_enroll_profile *p) {
    room_setup_guard *g=v;
    return bridge_guard_time(g) && g->store->write(g->store->ctx,p);
}
static inline bool bridge_read(void *v,room_enroll_profile *p) {
    room_setup_guard *g=v;
    return g->store->read(g->store->ctx,p);
}
static inline bool bridge_live(void *v,uint64_t trial) {
    room_setup_guard *g=v;
    return bridge_guard_time(g) && g->live && g->live(g->ctx,trial) && bridge_guard_time(g);
}
/* Returns a nonzero local reply ticket only when a reply may be sent. No
 * internal queue or credential copy. The caller owns/disposes input/output.
 * Clock is sampled again after dispatch; the deadline is never extended. */
static inline uint64_t bridge_request(room_setup_bridge *b,uint64_t window,uint64_t session,
    const uint8_t *in,size_t len,uint8_t out[ROOM_SETUP_REPLY_SIZE],const room_enroll_store *store,
    room_setup_clock clock,room_setup_live live,void *ctx) {
    if (!out) return 0;
    memset(out,0,ROOM_SETUP_REPLY_SIZE);
    if (!clock) { bridge_stop(b); return 0; }
    uint64_t now=clock(ctx); bridge_tick(b,now);
    if (!window || window!=b->window || b->stopping || !phone_authorized(&b->phone,session)) return 0;
    uint64_t t=setup_life_begin(&b->life,now,session);
    if (!t) { if (b->life.phase==SETUP_STOP) bridge_stop(b); return 0; }
    room_setup_guard g={b,store,clock,live,ctx,now,false};
    room_enroll_store guarded={&g,bridge_ready,bridge_write,bridge_read};
    bool reply=room_setup_handle(&b->dispatch,&b->phone,now,session,in,len,out,&guarded,bridge_live,&g);
    uint64_t end=clock(ctx);
    if (g.bad_clock || end<g.seen) b->life.failed=true;
    setup_life_tick(&b->life,end);
    bool closed=!b->phone.window;
    if (!closed) { phone_tick(&b->phone,end); if (!b->phone.window) reply=false; }
    if (b->phone.drain && !closed && out[2]!=3) reply=false;
    if (!reply || b->life.failed || b->life.phase!=SETUP_BUSY ||
        (closed && !setup_life_final(&b->life,end,session,t))) {
        memset(out,0,ROOM_SETUP_REPLY_SIZE); bridge_stop(b); return 0;
    }
    return t;
}
static inline bool bridge_sent(room_setup_bridge *b,uint64_t now,uint64_t window,uint64_t session,uint64_t t,bool ok) {
    bridge_tick(b,now);
    if (!window || window!=b->window || b->stopping) return false;
    bool done=setup_life_sent(&b->life,now,session,t,ok);
    if (b->life.phase==SETUP_STOP) bridge_stop(b);
    return done;
}
static inline void bridge_lost(room_setup_bridge *b,uint64_t now,uint64_t window,uint64_t session) {
    bridge_tick(b,now);
    if (window && window==b->window && session && session==b->life.session) bridge_stop(b);
}
/* Trusted owner acknowledges only after actual full transport and trial drain. */
static inline bool bridge_drained(room_setup_bridge *b,uint64_t now,uint64_t window) {
    bridge_tick(b,now);
    if (!window || window!=b->window || !b->stopping) return false;
    if (b->life.phase!=SETUP_DOWN && !setup_life_drained(&b->life,now,b->life.session)) return false;
    phone_drained(&b->phone); b->window=0; b->stopping=false; return true;
}
#endif
