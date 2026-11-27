#define _POSIX_C_SOURCE 200809L
#include "room_phone_channel.h"
#include "completion_adapter.h"
#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <time.h>

enum { ROUNDS=1000 };
static ack_adapter adapter;
static atomic_uint submitted, waiting;
static _Thread_local int task_id;
static unsigned good, bad;
static uint8_t reply_byte;
static unsigned expected=1;
static bool published;
static bool monotonic(void *ctx,uint64_t *ms) {
    (void)ctx; struct timespec t;
    if(clock_gettime(CLOCK_MONOTONIC,&t)!=0 || t.tv_sec<0) return false;
    *ms=(uint64_t)t.tv_sec*1000u+(uint64_t)t.tv_nsec/1000000u; return true;
}
static ack_result step(mb_event e,bool receipt,mb_reply *out) {
    e.window=1; return ack_step(&adapter,e,receipt,out,monotonic,NULL);
}
/* Retrying is confined to the offline harness, never the owner callbacks. */
static ack_result retry(mb_event e,bool receipt,mb_reply *out) {
    for(;;) { ack_result r=step(e,receipt,out); if(r!=ACK_BUSY && r!=ACK_WAIT) return r; sched_yield(); }
}
void *xTaskGetCurrentTaskHandle(void) { return &task_id; }
bool room_apsta_ready(room_apsta_owner *r,uint64_t w) { return r->ap && w; }
void enroll_wipe(void *p,size_t n) { volatile uint8_t *b=p; while(n--) *b++=0; }
bool room_apsta_task_poll(room_apsta_task *t) { return !t->failed; }
bool room_apsta_task_packet(room_apsta_task *t,bool h,const uint8_t *p,size_t n,const uint8_t **r,size_t *len) {
    assert(h && n==1 && p[0]==(uint8_t)expected && !t->session.transport.pending);
    reply_byte=(uint8_t)(p[0]^0x5a); *r=&reply_byte; *len=1; t->session.transport.pending=true; return true;
}
bool room_apsta_task_sent(room_apsta_task *t,bool ok) {
    assert(t->session.transport.pending); t->session.transport.pending=false;
    if(ok) { ++good; ++expected; published=false; } else ++bad; return true;
}
static bool open_io(void *p,const esp_netif_t *ap,uint64_t w) {
    (void)p; mb_reply out;
    return ap && w==1 && step((mb_event){.action=MB_OPEN},false,&out)==ACK_OK && step((mb_event){.action=MB_STARTED,.flag=true},false,&out)==ACK_OK;
}
static room_phone_io_result receive_io(void *p,bool *h,uint8_t *b,size_t cap,size_t *n) {
    (void)p; mb_reply out; *n=0;
    if(atomic_load_explicit(&submitted,memory_order_acquire)<expected) return ROOM_PHONE_IO_WAIT;
    ack_result r=step((mb_event){.action=MB_TAKE_REQUEST,.ticket=expected,.output=b,.size=cap},false,&out);
    if(r==ACK_BUSY) return ROOM_PHONE_IO_WAIT;
    if(r!=ACK_OK) return ROOM_PHONE_IO_ERROR;
    *n=out.length; *h=out.handshake; return ROOM_PHONE_IO_OK;
}
static room_phone_io_result send_io(void *p,const uint8_t *b,size_t n,size_t *written) {
    (void)p; mb_reply out; *written=0;
    if(!published) {
        ack_result r=step((mb_event){.action=MB_PUBLISH,.ticket=expected,.input=b,.size=n},false,&out);
        if(r==ACK_BUSY) return ROOM_PHONE_IO_WAIT;
        if(r!=ACK_OK) return ROOM_PHONE_IO_ERROR;
        published=true; return ROOM_PHONE_IO_WAIT;
    }
    ack_result r=step((mb_event){.ticket=expected},true,&out);
    if(r==ACK_BUSY) return ROOM_PHONE_IO_WAIT;
    if(r==ACK_WAIT) { atomic_store_explicit(&waiting,expected,memory_order_release); return ROOM_PHONE_IO_WAIT; }
    if(r!=ACK_OK || out.ticket!=expected) return ROOM_PHONE_IO_ERROR;
    *written=n; return ROOM_PHONE_IO_OK;
}
static bool close_io(void *p) {
    (void)p; mb_reply out;
    return step((mb_event){.action=MB_STOP},false,&out)==ACK_OK && step((mb_event){.action=MB_DRAINED,.flag=true,.drained=true},false,&out)==ACK_OK;
}
static void *worker(void *unused) {
    (void)unused; mb_reply out;
    for(unsigned i=1;i<=ROUNDS;i++) {
        uint8_t in=(uint8_t)i, copy=0;
        assert(retry((mb_event){.action=MB_SUBMIT,.input=&in,.size=1,.flag=true},false,&out)==ACK_OK && out.ticket==i);
        atomic_store_explicit(&submitted,i,memory_order_release);
        /* A not-yet-published reply is a normal synthetic scheduling state. */
        while(step((mb_event){.action=MB_TAKE_REPLY,.ticket=i,.output=&copy,.size=1},false,&out)!=ACK_OK) sched_yield();
        assert(out.ticket==i && out.length==1 && copy==(uint8_t)(in^0x5a));
        while(atomic_load_explicit(&waiting,memory_order_acquire)<i) sched_yield();
        assert(retry((mb_event){.action=MB_SENT,.ticket=i,.flag=true},false,&out)==ACK_OK);
    }
    return NULL;
}
int main(void) {
    ack_init(&adapter); atomic_init(&submitted,0); atomic_init(&waiting,0);
    room_phone_channel c={0}; room_apsta_task t={0}; esp_netif_t ap={0};
    t.ready=true; t.owner=xTaskGetCurrentTaskHandle(); t.session.radio.ap=&ap; t.session.bridge.window=1;
    assert(room_phone_channel_open(&c,&t,(room_phone_io){NULL,open_io,receive_io,send_io,close_io}));
    pthread_t thread; assert(pthread_create(&thread,NULL,worker,NULL)==0);
    while(good<ROUNDS) { assert(room_phone_channel_poll(&c)); sched_yield(); }
    assert(pthread_join(thread,NULL)==0);
    assert(!bad && !c.reply && !t.session.transport.pending && expected==ROUNDS+1);
    assert(room_phone_channel_close(&c) && !c.task);
    printf("offline actual channel: %u two-thread exchanges PASS\n",good); return 0;
}
