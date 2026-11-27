#ifndef ROOM_DRAIN_H
#define ROOM_DRAIN_H
#include <stdbool.h>
#include <stdint.h>
/* Offline owner-only contract. Observations are trusted local facts, not
 * callback admission itself. One context per window; never reset while live. */
typedef enum { DRAIN_IDLE, DRAIN_WAIT, DRAIN_DONE, DRAIN_LOCKED } room_drain_state;
typedef struct { uint64_t window, deadline, last; room_drain_state state; } room_drain;
static inline bool room_drain_begin(room_drain *d, uint64_t w, uint64_t now, uint64_t deadline) {
    if (!d || d->state==DRAIN_LOCKED) return false;
    if (d->state!=DRAIN_IDLE) {
        if (d->state==DRAIN_DONE) return false;
        if (d->window!=w || d->deadline!=deadline || now<d->last || now>=deadline) { d->state=DRAIN_LOCKED; return false; }
        d->last=now; return true;
    }
    if (!w || deadline<=now || deadline-now>5000u) { d->state=DRAIN_LOCKED; return false; }
    d->window=w; d->deadline=deadline; d->last=now; d->state=DRAIN_WAIT; return true;
}
static inline room_drain_state room_drain_step(room_drain *d, uint64_t w, uint64_t now, bool sdk_ok, bool sdk_error, bool admission_closed, bool callbacks_done, bool buffers_done) {
    if (!d) return DRAIN_LOCKED;
    if (d->state==DRAIN_LOCKED) return DRAIN_LOCKED;
    if (d->state==DRAIN_IDLE || w!=d->window || now<d->last || sdk_error) return d->state=DRAIN_LOCKED;
    if (d->state==DRAIN_DONE) return DRAIN_DONE;
    if (now>=d->deadline) return d->state=DRAIN_LOCKED;
    d->last=now;
    if (sdk_ok && admission_closed && callbacks_done && buffers_done) d->state=DRAIN_DONE;
    return d->state;
}
#endif
