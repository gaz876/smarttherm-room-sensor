#ifndef ROOM_TIMED_COMPLETION_ADAPTER_H
#define ROOM_TIMED_COMPLETION_ADAPTER_H
#include "completion_adapter.h"

/* Offline owner-budget mode. Initialize once, before sharing. Transaction
 * expiry closes admission but cannot set or extend the cleanup deadline.
 * Only the trusted owner may call close_until; proof must include external
 * worker exit and every borrowed buffer/SDK lease, not just mailbox emptiness.
 * A failed proof never frees external storage. No live I/O under the lock. */
static inline void ack_init_owner_budget(ack_adapter *a) {
    ack_init(a); a->box.life.owner_budget=true;
}
static inline ack_result ack_close_until(ack_adapter *a,uint64_t window,
    uint64_t deadline,bool exited,bool drained,mb_clock clock,void *ctx) {
    if(!a||!window||!clock) return ACK_REJECT;
    bool expected=false;
    if(!atomic_compare_exchange_strong_explicit(&a->held,&expected,true,memory_order_acquire,memory_order_relaxed)) return ACK_BUSY;
    ack_result rc=ACK_REJECT;
    mailbox *m=&a->box; worker_model *w=&m->life;
    if(!w->owner_budget||w->window!=window||w->phase==W_DOWN||w->phase==W_LOCKED) goto done;
    uint64_t now=0; mb_reply out;
    if(!clock(ctx,&now)||!deadline||now>=deadline||deadline-now>5000u||
       (w->clock_seen&&now<w->last_time)||w->fault||
       (w->stop_deadline&&w->stop_deadline!=deadline)) goto lock;
    if(mb_apply(m,(mb_event){.action=MB_STOP,.window=window,.now=now},&out)!=MB_OK||w->phase!=W_STOP||w->fault) goto lock;
    w->stop_deadline=deadline;
    ack_clear(a);
    rc=ACK_WAIT;
    if(exited&&drained) {
        if(mb_apply(m,(mb_event){.action=MB_DRAINED,.window=window,.now=now,.flag=true,.drained=true},&out)!=MB_OK) goto lock;
        rc=ACK_OK;
    }
    goto done;
lock:
    w->phase=W_LOCKED; w->fault=true; mb_clear(m); m->taken=false; m->handshake=false; ack_clear(a);
    rc=ACK_REJECT;
done:
    atomic_store_explicit(&a->held,false,memory_order_release);
    return rc;
}
#endif
