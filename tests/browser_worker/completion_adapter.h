#ifndef ROOM_COMPLETION_ADAPTER_H
#define ROOM_COMPLETION_ADAPTER_H
#include "mailbox.h"

/* Offline mock only. All access, including ticks, goes through this adapter.
 * The outer try-lock serializes mailbox calls and the copied receipt. The inner
 * mailbox lock is uncontended by construction. No live I/O or retained pointer.
 * A receipt is not external-buffer drain or worker exit. */
typedef enum { ACK_BUSY, ACK_WAIT, ACK_REJECT, ACK_OK } ack_result;
typedef struct {
    atomic_bool held;
    mailbox box;
    uint64_t window, ticket, deadline;
} ack_adapter;
static inline void ack_init(ack_adapter *a) {
    atomic_init(&a->held,false); mb_init(&a->box);
    a->window=a->ticket=a->deadline=0;
}
static inline void ack_clear(ack_adapter *a) {
    a->window=a->ticket=a->deadline=0;
}
/* receipt=true is an owner poll, otherwise forward a trusted local event.
 * The clock is sampled only under the outer lock, once per operation. */
static inline ack_result ack_step(ack_adapter *a,mb_event e,bool receipt,
                                  mb_reply *out,mb_clock clock,void *ctx) {
    if(!a || !out || !clock) return ACK_REJECT;
    *out=(mb_reply){0};
    bool expected=false;
    if(!atomic_compare_exchange_strong_explicit(&a->held,&expected,true,memory_order_acquire,memory_order_relaxed)) return ACK_BUSY;
    ack_result result=ACK_REJECT;
    mailbox *m=&a->box;
    if(!clock(ctx,&e.now)) {
        m->life.fault=true;
        (void)mb_apply(m,(mb_event){.action=MB_STOP,.window=m->life.window,.now=m->life.last_time},out);
        ack_clear(a); goto done;
    }
    (void)mb_apply(m,(mb_event){.action=MB_TICK,.now=e.now},out);
    if(a->ticket && e.now>=a->deadline) {
        (void)mb_apply(m,(mb_event){.action=MB_STOP,.window=a->window,.now=e.now},out);
        ack_clear(a);
    }
    if(m->life.phase!=W_IDLE) ack_clear(a);
    if(receipt) {
        if(a->ticket && e.window==a->window && e.ticket==a->ticket) {
            out->ticket=a->ticket; ack_clear(a); result=ACK_OK;
        } else if(!a->ticket && e.window==m->life.window && e.ticket && e.ticket==m->life.ticket && m->life.phase==W_REPLY) {
            result=ACK_WAIT;
        }
        goto done;
    }
    if(a->ticket && e.action==MB_SUBMIT) { result=ACK_WAIT; goto done; }
    if(m->life.owner_budget && e.action==MB_DRAINED) goto done;
    uint64_t deadline=m->life.deadline;
    mb_result rc=mb_apply(m,e,out);
    if(rc==MB_OK) {
        result=ACK_OK;
        if(e.action==MB_SENT) { a->window=e.window; a->ticket=e.ticket; a->deadline=deadline; }
    }
    if(m->life.phase!=W_IDLE) ack_clear(a);
done:
    atomic_store_explicit(&a->held,false,memory_order_release);
    return result;
}
#endif
