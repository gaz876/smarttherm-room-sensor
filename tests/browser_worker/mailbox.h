#ifndef ROOM_MAILBOX_H
#define ROOM_MAILBOX_H
#include "worker_model.h"
#include <stdatomic.h>
#include <string.h>

/* Host-only copied mailbox. Initialize once before sharing; never move it.
 * Trusted callers serialize timestamps at the lock boundary (same clock).
 * No callbacks, I/O, allocation or waiting inside the try-lock. BUSY means
 * retry later, not success. External copies remain the caller's responsibility.
 * This is not an SDK worker or a proof of AP isolation/quiescence. */
enum { MB_CAP=4096 };
typedef enum { MB_BUSY, MB_REJECT, MB_OK } mb_result;
typedef enum { MB_OPEN, MB_STARTED, MB_SUBMIT, MB_TAKE_REQUEST, MB_PUBLISH,
    MB_TAKE_REPLY, MB_SENT, MB_STOP, MB_DRAINED, MB_TICK } mb_action;
typedef struct {
    atomic_bool held;
    worker_model life;
    uint8_t data[MB_CAP];
    size_t length;
    bool taken, handshake;
} mailbox;
typedef struct {
    mb_action action;
    uint64_t now, window, ticket;
    const uint8_t *input;
    uint8_t *output;
    size_t size;
    bool flag, drained;
} mb_event;
typedef struct { uint64_t ticket; size_t length; bool handshake; } mb_reply;
static inline void mb_init(mailbox *m) {
    atomic_init(&m->held,false); m->life=(worker_model){0};
    memset(m->data,0,sizeof m->data); m->length=0; m->taken=false; m->handshake=false;
}
static inline void mb_clear(mailbox *m) {
    volatile uint8_t *p=m->data;
    for(size_t i=0;i<sizeof m->data;i++) p[i]=0;
    m->length=0;
}
/* Input/output must be private caller buffers, disjoint from mailbox and reply.
 * No pointer retained. A copied response is NOT a send acknowledgement. */
static inline mb_result mb_apply(mailbox *m,mb_event e,mb_reply *out) {
    if (!m || !out) return MB_REJECT;
    *out=(mb_reply){0};
    bool expected=false;
    if (!atomic_compare_exchange_strong_explicit(&m->held,&expected,true,memory_order_acquire,memory_order_relaxed)) return MB_BUSY;
    worker_model *w=&m->life;
    worker_tick(w,e.now);
    bool ok=false;
    bool current=e.window && e.window==w->window;
    bool ticket=current && e.ticket && e.ticket==w->ticket;
    switch(e.action) {
    case MB_OPEN: ok=worker_open(w,e.now,e.window); break;
    case MB_STARTED: ok=worker_started(w,e.now,e.window,e.flag); break;
    case MB_SUBMIT:
        if(e.input && e.size && e.size<=MB_CAP && (out->ticket=worker_request(w,e.now,e.window,e.size))) {
            memcpy(m->data,e.input,e.size); m->length=e.size; m->taken=false; m->handshake=e.flag; ok=true;
        } break;
    case MB_TAKE_REQUEST:
    case MB_TAKE_REPLY:
        if(ticket && !m->taken && m->length && e.output && e.size>=m->length &&
           ((e.action==MB_TAKE_REQUEST && w->phase==W_REQUEST)||(e.action==MB_TAKE_REPLY && w->phase==W_REPLY))) {
            memcpy(e.output,m->data,m->length); out->ticket=w->ticket; out->length=m->length;
            out->handshake=m->handshake; mb_clear(m); m->taken=true; ok=true;
        } break;
    case MB_PUBLISH:
        if(ticket && m->taken && w->phase==W_REQUEST && e.input && e.size && e.size<=MB_CAP &&
           worker_reply(w,e.now,e.window,e.ticket,e.size)) {
            memcpy(m->data,e.input,e.size); m->length=e.size; m->taken=false; ok=true;
        } break;
    case MB_SENT:
        if(ticket && m->taken && w->phase==W_REPLY) {
            ok=worker_sent(w,e.now,e.window,e.ticket,e.flag); m->taken=false;
        } break;
    case MB_STOP: if(current) { worker_stop(w,e.now); ok=true; } break;
    case MB_DRAINED: ok=worker_drained(w,e.now,e.window,e.flag,e.drained); break;
    case MB_TICK: ok=true; break;
    }
    if(w->phase==W_STOP || w->phase==W_LOCKED || w->phase==W_DOWN) {
        mb_clear(m); m->taken=false; m->handshake=false;
    }
    atomic_store_explicit(&m->held,false,memory_order_release);
    return ok?MB_OK:MB_REJECT;
}
#endif
