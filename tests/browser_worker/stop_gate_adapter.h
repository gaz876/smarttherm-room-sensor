#ifndef ROOM_STOP_GATE_ADAPTER_H
#define ROOM_STOP_GATE_ADAPTER_H
#include "timed_completion_adapter.h"

/* Offline prototype. One owner serializes open/close; all traffic uses
 * gate_step. No direct access to inner adapter after sharing. STOP is
 * published even while a traffic operation owns BUSY. Operations admitted
 * before STOP may finish copying, but their result is discarded if STOP
 * wins the atomic release. External SDK/buffer drain is still required. */
enum { GATE_BUSY=1u, GATE_STOP=2u };
typedef struct {
    atomic_uint gate;
    ack_adapter inner;
    uint64_t window, deadline, last_close; /* owner-only */
    bool fault; /* owner-only sticky quarantine */
} stop_gate_adapter;
static inline void gate_init(stop_gate_adapter *g) {
    atomic_init(&g->gate,GATE_STOP); ack_init_owner_budget(&g->inner);
    g->window=g->deadline=g->last_close=0; g->fault=false;
}
static inline bool gate_open(stop_gate_adapter *g,uint64_t w,mb_clock clock,void *ctx) {
    if(!g||!w||!clock||g->window||g->fault||atomic_load(&g->gate)!=GATE_STOP) return false;
    mb_reply out;
    if(ack_step(&g->inner,(mb_event){.action=MB_OPEN,.window=w},false,&out,clock,ctx)!=ACK_OK) return false;
    g->window=w;
    if(ack_step(&g->inner,(mb_event){.action=MB_STARTED,.window=w,.flag=true},false,&out,clock,ctx)!=ACK_OK) return false;
    atomic_store(&g->gate,0u); return true;
}
static inline ack_result gate_step(stop_gate_adapter *g,mb_event e,bool receipt,mb_reply *out,mb_clock clock,void *ctx) {
    if(!g||!out||!clock) return ACK_REJECT;
    *out=(mb_reply){0};
    if(!receipt&&(e.action==MB_OPEN||e.action==MB_STARTED||e.action==MB_STOP||e.action==MB_DRAINED)) return ACK_REJECT;
    unsigned expected=0;
    if(!atomic_compare_exchange_strong(&g->gate,&expected,GATE_BUSY)) return (expected&GATE_STOP)?ACK_REJECT:ACK_BUSY;
    ack_result rc=ack_step(&g->inner,e,receipt,out,clock,ctx);
    expected=GATE_BUSY;
    if(!atomic_compare_exchange_strong(&g->gate,&expected,0u)) {
        if(!receipt&&(e.action==MB_TAKE_REQUEST||e.action==MB_TAKE_REPLY)&&e.output&&out->length<=e.size) {
            volatile uint8_t *p=e.output; for(size_t i=0;i<out->length;i++)p[i]=0;
        }
        *out=(mb_reply){0};
        atomic_fetch_and(&g->gate,~(unsigned)GATE_BUSY); return ACK_REJECT;
    }
    return rc;
}
static inline ack_result gate_close_until(stop_gate_adapter *g,uint64_t w,uint64_t deadline,bool exited,bool drained,mb_clock clock,void *ctx) {
    if(!g||!w||w!=g->window) return ACK_REJECT;
    unsigned prior=atomic_fetch_or(&g->gate,GATE_STOP);
    uint64_t now=0;
    if(!clock||!clock(ctx,&now)||!deadline||now>=deadline||deadline-now>5000u||
       now<g->last_close||(g->deadline&&g->deadline!=deadline))g->fault=true;
    g->last_close=now;
    if(!g->deadline)g->deadline=deadline;
    if(g->fault)return ACK_REJECT;
    if(prior&GATE_BUSY)return ACK_BUSY;
    ack_result rc=ack_close_until(&g->inner,w,deadline,exited,drained,clock,ctx);
    if(rc==ACK_OK){g->window=g->deadline=g->last_close=0;}
    else if(rc==ACK_REJECT)g->fault=true;
    return rc;
}
#endif
