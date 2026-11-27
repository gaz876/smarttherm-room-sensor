#ifndef ROOM_WORKER_MODEL_H
#define ROOM_WORKER_MODEL_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Offline serialized event model, not a server or synchronization primitive.
 * All IDs and events come from trusted local actors, not network fields.
 * A timeout requests shutdown; it never proves resource release. */
typedef enum { W_DOWN, W_START, W_IDLE, W_REQUEST, W_REPLY, W_STOP, W_LOCKED } worker_phase;
typedef struct {
    worker_phase phase;
    uint64_t window, last_window, ticket, deadline, stop_deadline, last_time;
    bool clock_seen, fault, owner_budget;
} worker_model;
static inline uint64_t worker_limit(uint64_t now, uint64_t duration) {
    return now > UINT64_MAX-duration ? UINT64_MAX : now+duration;
}
static inline void worker_stop(worker_model *w, uint64_t now) {
    if (w->phase==W_DOWN || w->phase==W_STOP || w->phase==W_LOCKED) return;
    w->phase=W_STOP; w->deadline=0; w->stop_deadline=w->owner_budget?0:worker_limit(now,2000);
}
static inline void worker_tick(worker_model *w, uint64_t now) {
    if (w->clock_seen && now<w->last_time) {
        w->fault=true; worker_stop(w,w->last_time); return;
    }
    w->clock_seen=true; w->last_time=now;
    if ((w->phase==W_START || w->phase==W_REQUEST || w->phase==W_REPLY) && now>=w->deadline)
        worker_stop(w,now);
    if (w->phase==W_STOP && w->stop_deadline && now>=w->stop_deadline) {
        w->phase=W_LOCKED; w->fault=true;
    }
}
static inline bool worker_open(worker_model *w,uint64_t now,uint64_t window) {
    worker_tick(w,now);
    if (w->fault || w->phase!=W_DOWN || !window || window<=w->last_window || now==UINT64_MAX) return false;
    w->window=w->last_window=window; w->phase=W_START; w->deadline=worker_limit(now,2000); return true;
}
static inline bool worker_started(worker_model *w,uint64_t now,uint64_t window,bool ok) {
    worker_tick(w,now);
    if (w->phase!=W_START || window!=w->window) return false;
    if (!ok) { worker_stop(w,now); return false; }
    w->phase=W_IDLE; w->deadline=0; return true;
}
static inline uint64_t worker_request(worker_model *w,uint64_t now,uint64_t window,size_t length) {
    worker_tick(w,now);
    if (w->phase!=W_IDLE || window!=w->window || !length || length>4096) return 0;
    if (w->ticket==UINT64_MAX || now==UINT64_MAX) { w->fault=true; worker_stop(w,now); return 0; }
    w->phase=W_REQUEST; w->deadline=worker_limit(now,2000); return ++w->ticket;
}
static inline bool worker_reply(worker_model *w,uint64_t now,uint64_t window,uint64_t ticket,size_t length) {
    worker_tick(w,now);
    if (w->phase!=W_REQUEST || window!=w->window || !ticket || ticket!=w->ticket) return false;
    if (!length || length>4096) { worker_stop(w,now); return false; }
    w->phase=W_REPLY; return true;
}
static inline bool worker_sent(worker_model *w,uint64_t now,uint64_t window,uint64_t ticket,bool ok) {
    worker_tick(w,now);
    if (w->phase!=W_REPLY || window!=w->window || !ticket || ticket!=w->ticket) return false;
    if (!ok) { worker_stop(w,now); return false; }
    w->phase=W_IDLE; w->deadline=0; return true;
}
/* Trusted supervisor reports actual worker exit AND callback/buffer drain.
 * Even a late valid report cannot unlock a timed-out worker for reuse. */
static inline bool worker_drained(worker_model *w,uint64_t now,uint64_t window,bool exited,bool drained) {
    worker_tick(w,now);
    if (w->phase!=W_STOP || window!=w->window || !exited || !drained || !w->stop_deadline || w->fault) return false;
    w->phase=W_DOWN; w->window=0; w->stop_deadline=0; return true;
}
#endif
