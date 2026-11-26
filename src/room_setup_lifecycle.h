#ifndef ROOM_SETUP_LIFECYCLE_H
#define ROOM_SETUP_LIFECYCLE_H
#include <stdbool.h>
#include <stdint.h>

/* Experimental single-owner lifecycle model, no SDK calls. Session identity
 * is supplied by the trusted transport. Tickets never originate on the wire.
 * Stop means reject new input and request teardown; quiescence must be proved
 * by the actual transport before reuse. A send completion is not peer receipt. */
typedef enum { SETUP_DOWN, SETUP_OPEN, SETUP_BUSY, SETUP_FINAL, SETUP_STOP } room_setup_phase;
typedef struct {
    room_setup_phase phase;
    uint64_t session, last_session, ticket, deadline, last_ms;
    bool clock_seen, failed;
} room_setup_lifecycle;
static inline uint64_t setup_life_limit(uint64_t now) {
    return now>UINT64_MAX-2000u?UINT64_MAX:now+2000u;
}
static inline void setup_life_stop(room_setup_lifecycle *s) {
    if (s->phase!=SETUP_DOWN) { s->phase=SETUP_STOP; s->deadline=0; }
}
static inline void setup_life_tick(room_setup_lifecycle *s,uint64_t now) {
    if (s->clock_seen && now<s->last_ms) { s->failed=true; setup_life_stop(s); return; }
    s->clock_seen=true; s->last_ms=now;
    if ((s->phase==SETUP_BUSY || s->phase==SETUP_FINAL) && now>=s->deadline)
        setup_life_stop(s);
}
static inline bool setup_life_open(room_setup_lifecycle *s,uint64_t now,uint64_t session) {
    setup_life_tick(s,now);
    if (s->failed || s->phase!=SETUP_DOWN || !session || session<=s->last_session || now==UINT64_MAX) return false;
    s->session=session; s->last_session=session; s->phase=SETUP_OPEN; return true;
}
/* Exactly one pending command/response. No unbounded queue. */
static inline uint64_t setup_life_begin(room_setup_lifecycle *s,uint64_t now,uint64_t session) {
    setup_life_tick(s,now);
    if (s->phase!=SETUP_OPEN || session!=s->session) return 0;
    if (s->ticket==UINT64_MAX || now==UINT64_MAX) { s->failed=true; setup_life_stop(s); return 0; }
    s->phase=SETUP_BUSY; s->deadline=setup_life_limit(now); return ++s->ticket;
}
/* Called after dispatch closes its logical window, before sending final reply.
 * Retains the original deadline, so a slow handler cannot extend the budget. */
static inline bool setup_life_final(room_setup_lifecycle *s,uint64_t now,uint64_t session,uint64_t ticket) {
    setup_life_tick(s,now);
    if (s->phase!=SETUP_BUSY || session!=s->session || !ticket || ticket!=s->ticket) return false;
    s->phase=SETUP_FINAL; return true;
}
static inline bool setup_life_sent(room_setup_lifecycle *s,uint64_t now,uint64_t session,uint64_t ticket,bool ok) {
    setup_life_tick(s,now);
    if ((s->phase!=SETUP_BUSY && s->phase!=SETUP_FINAL) || session!=s->session || !ticket || ticket!=s->ticket) return false;
    if (!ok || s->phase==SETUP_FINAL) setup_life_stop(s);
    else { s->phase=SETUP_OPEN; s->deadline=0; }
    return true;
}
static inline void setup_life_lost(room_setup_lifecycle *s,uint64_t now,uint64_t session) {
    setup_life_tick(s,now);
    if (session && session==s->session) setup_life_stop(s);
}
/* Only a trusted owner may acknowledge after AP/STA/session cleanup AND
 * draining callbacks, not merely after requesting stop. */
static inline bool setup_life_drained(room_setup_lifecycle *s,uint64_t now,uint64_t session) {
    setup_life_tick(s,now);
    if (s->phase!=SETUP_STOP || !session || session!=s->session) return false;
    s->phase=SETUP_DOWN; s->session=0; s->deadline=0; return true;
}
#endif
