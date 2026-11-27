#define _POSIX_C_SOURCE 200809L
#include "room_phone_channel.h"
#include "completion_adapter.h"
#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>

/* Offline fixture: logical time and deliberately held clock callback model
 * scheduling, not a bounded production clock or an actual SDK worker. */
static ack_adapter a;
static atomic_bool submitted, copied, hold_clock, in_clock, release_clock, finish, wiped;
static atomic_uint_fast64_t now_ms;
static _Thread_local int task_id;
static _Thread_local bool worker_context;
static bool joined, published;
static unsigned good, bad, checks;
static uint8_t response;
#define CHECK(x) do { assert(x); ++checks; } while (0)
static void wait_flag(atomic_bool *p) { while(!atomic_load(p)) sched_yield(); }
static bool clock_read(void *ctx,uint64_t *ms) {
    (void)ctx;
    if(worker_context && atomic_load(&hold_clock)) {
        atomic_store(&in_clock,true); wait_flag(&release_clock);
    }
    *ms=atomic_load(&now_ms); return true;
}
static ack_result step(mb_event e,bool receipt,mb_reply *out) {
    e.window=1; return ack_step(&a,e,receipt,out,clock_read,NULL);
}
void *xTaskGetCurrentTaskHandle(void) { return &task_id; }
bool room_apsta_ready(room_apsta_owner *r,uint64_t w) { return r->ap && w==1; }
void enroll_wipe(void *p,size_t n) { volatile uint8_t *b=p; while(n--) *b++=0; }
bool room_apsta_task_poll(room_apsta_task *t) { return !t->failed; }
bool room_apsta_task_packet(room_apsta_task *t,bool h,const uint8_t *p,size_t n,const uint8_t **r,size_t *len) {
    assert(h && n==1 && !t->session.transport.pending);
    response=(uint8_t)(p[0]+1u); *r=&response; *len=1; t->session.transport.pending=true; return true;
}
bool room_apsta_task_sent(room_apsta_task *t,bool ok) {
    assert(t->session.transport.pending); t->session.transport.pending=false;
    if(ok) ++good; else ++bad; return true;
}
static bool open_io(void *p,const esp_netif_t *ap,uint64_t w) {
    (void)p; mb_reply out;
    return ap && w==1 && step((mb_event){.action=MB_OPEN},false,&out)==ACK_OK && step((mb_event){.action=MB_STARTED,.flag=true},false,&out)==ACK_OK;
}
static room_phone_io_result receive_io(void *p,bool *h,uint8_t *b,size_t cap,size_t *n) {
    (void)p; mb_reply out; *n=0;
    if(!atomic_load(&submitted)) return ROOM_PHONE_IO_WAIT;
    ack_result r=step((mb_event){.action=MB_TAKE_REQUEST,.ticket=1,.output=b,.size=cap},false,&out);
    if(r==ACK_BUSY) return ROOM_PHONE_IO_WAIT;
    if(r!=ACK_OK) return ROOM_PHONE_IO_ERROR;
    *h=out.handshake; *n=out.length; return ROOM_PHONE_IO_OK;
}
static room_phone_io_result send_io(void *p,const uint8_t *b,size_t n,size_t *written) {
    (void)p; mb_reply out; *written=0;
    ack_result r;
    if(!published) {
        r=step((mb_event){.action=MB_PUBLISH,.ticket=1,.input=b,.size=n},false,&out);
        if(r==ACK_BUSY) return ROOM_PHONE_IO_WAIT;
        if(r!=ACK_OK) return ROOM_PHONE_IO_ERROR;
        published=true; return ROOM_PHONE_IO_WAIT;
    }
    r=step((mb_event){.ticket=1},true,&out);
    if(r==ACK_BUSY || r==ACK_WAIT) return ROOM_PHONE_IO_WAIT;
    if(r!=ACK_OK || out.ticket!=1) return ROOM_PHONE_IO_ERROR;
    *written=n; return ROOM_PHONE_IO_OK;
}
static bool close_io(void *p) {
    (void)p; mb_reply out;
    if(step((mb_event){.action=MB_STOP},false,&out)!=ACK_OK) return false;
    /* Only an actual successful join is accepted as exit proof. */
    if(!joined || !atomic_load(&wiped)) return false;
    return step((mb_event){.action=MB_DRAINED,.flag=joined,.drained=atomic_load(&wiped)},false,&out)==ACK_OK;
}
static void *worker(void *unused) {
    (void)unused; worker_context=true; mb_reply out; uint8_t data=12, copy=0;
    assert(step((mb_event){.action=MB_SUBMIT,.input=&data,.size=1,.flag=true},false,&out)==ACK_OK && out.ticket==1);
    atomic_store(&submitted,true);
    while(step((mb_event){.action=MB_TAKE_REPLY,.ticket=1,.output=&copy,.size=1},false,&out)!=ACK_OK) sched_yield();
    assert(copy==13 && out.length==1); atomic_store(&copied,true);
    wait_flag(&hold_clock);
    ack_result tick;
    do { tick=step((mb_event){.action=MB_TICK},false,&out); } while(tick==ACK_BUSY);
    assert(tick==ACK_OK);
    wait_flag(&finish);
    ack_result r;
    do { r=step((mb_event){.action=MB_SENT,.ticket=1,.flag=true},false,&out); } while(r==ACK_BUSY);
    assert(r==ACK_REJECT);
    enroll_wipe(&copy,sizeof copy); enroll_wipe(&data,sizeof data);
    assert(!copy && !data); atomic_store(&wiped,true); return NULL;
}
static void run(bool expiry,bool late) {
    ack_init(&a); atomic_init(&submitted,false); atomic_init(&copied,false);
    atomic_init(&hold_clock,false); atomic_init(&in_clock,false); atomic_init(&release_clock,false);
    atomic_init(&finish,false); atomic_init(&wiped,false); atomic_init(&now_ms,0);
    joined=published=false; good=bad=0;
    room_phone_channel c={0}; room_apsta_task t={0}; esp_netif_t ap={0};
    t.ready=true; t.owner=xTaskGetCurrentTaskHandle(); t.session.radio.ap=&ap; t.session.bridge.window=1;
    CHECK(room_phone_channel_open(&c,&t,(room_phone_io){NULL,open_io,receive_io,send_io,close_io}));
    pthread_t thread; CHECK(pthread_create(&thread,NULL,worker,NULL)==0);
    while(!atomic_load(&copied)) { assert(room_phone_channel_poll(&c)); sched_yield(); }
    CHECK(c.reply && t.session.transport.pending && good==0);
    atomic_store(&hold_clock,true); wait_flag(&in_clock);
    if(expiry) {
        atomic_store(&now_ms,2000);
        CHECK(room_phone_channel_poll(&c)); /* busy, not sent */
    } else {
        CHECK(!room_phone_channel_close(&c)); /* busy, not drained */
        CHECK(c.stopping);
    }
    CHECK(c.reply && t.session.transport.pending && !good && !bad);
    atomic_store(&release_clock,true);
    if(expiry) { while(room_phone_channel_poll(&c)) sched_yield(); CHECK(c.stopping); }
    /* Wait until the synthetic stop has reached the adapter. */
    mb_reply out; ack_result r;
    do { r=step((mb_event){.action=MB_STOP},false,&out); } while(r==ACK_BUSY);
    CHECK(r==ACK_OK);
    CHECK(!room_phone_channel_close(&c));
    CHECK(c.reply && t.session.transport.pending && !good && !bad);
    atomic_store(&finish,true); wait_flag(&wiped);
    CHECK(!room_phone_channel_close(&c)); /* wiped is not joined */
    CHECK(c.reply && t.session.transport.pending && !good && !bad);
    CHECK(pthread_join(thread,NULL)==0); joined=true;
    if(late) atomic_store(&now_ms,expiry?4000:2000);
    if(late) {
        CHECK(!room_phone_channel_close(&c));
        CHECK(c.reply && t.session.transport.pending && c.stopping && !good && !bad);
    } else {
        CHECK(room_phone_channel_close(&c));
        CHECK(!c.task && !c.reply && !t.session.transport.pending && !good && bad==1);
    }
}
int main(void) {
    run(false,false); run(true,false); run(false,true); run(true,true);
    printf("offline stop/expiry: 4 two-thread scenarios, %u checks PASS\n",checks); return 0;
}
