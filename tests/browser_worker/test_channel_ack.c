#include "room_phone_channel.h"
#include "completion_adapter.h"
#include <assert.h>
#include <stdio.h>

/* Serialized offline fixture; no network, SDK, or real sender. */
static unsigned checks, good, bad;
#define CHECK(x) do { assert(x); ++checks; } while(0)
static int owner_id;
static uint8_t response[]={9,8,7,6};
void *xTaskGetCurrentTaskHandle(void) { return &owner_id; }
bool room_apsta_ready(room_apsta_owner *r,uint64_t w) { return r->ap && w; }
void enroll_wipe(void *p,size_t n) { volatile uint8_t *b=p; while(n--) *b++=0; }
bool room_apsta_task_poll(room_apsta_task *t) { return !t->failed; }
bool room_apsta_task_packet(room_apsta_task *t,bool h,const uint8_t *p,size_t n,const uint8_t **r,size_t *len) {
    CHECK(h && n==1 && p[0]==42 && !t->session.transport.pending);
    t->session.transport.pending=true; *r=response; *len=sizeof response; return true;
}
bool room_apsta_task_sent(room_apsta_task *t,bool ok) {
    CHECK(t->session.transport.pending); t->session.transport.pending=false;
    if(ok) ++good; else ++bad; return true;
}
typedef struct {
    ack_adapter a;
    uint64_t now,w,ticket;
    bool received,published,stopping,exited,drained;
    size_t progressed;
    uint8_t copy[sizeof response];
} fixture;
static bool clock_read(void *p,uint64_t *ms) { *ms=((fixture *)p)->now; return true; }
static ack_result step(fixture *f,mb_event e,bool receipt,mb_reply *out) {
    e.window=f->w; return ack_step(&f->a,e,receipt,out,clock_read,f);
}
#define EV(f,a,t,...) step(f,(mb_event){.action=a,.ticket=t,__VA_ARGS__},false,&out)
static bool open_io(void *p,const esp_netif_t *ap,uint64_t w) {
    fixture *f=p; mb_reply out; f->w=w;
    return ap && EV(f,MB_OPEN,0,.flag=false)==ACK_OK && EV(f,MB_STARTED,0,.flag=true)==ACK_OK;
}
static room_phone_io_result receive_io(void *p,bool *h,uint8_t *b,size_t cap,size_t *n) {
    fixture *f=p; mb_reply out; *n=0;
    if(!f->ticket || f->received) return ROOM_PHONE_IO_WAIT;
    ack_result r=EV(f,MB_TAKE_REQUEST,f->ticket,.output=b,.size=cap);
    if(r==ACK_BUSY) return ROOM_PHONE_IO_WAIT;
    if(r!=ACK_OK) return ROOM_PHONE_IO_ERROR;
    *n=out.length; *h=out.handshake; f->received=true; return ROOM_PHONE_IO_OK;
}
static room_phone_io_result send_io(void *p,const uint8_t *b,size_t n,size_t *written) {
    fixture *f=p; mb_reply out; *written=0;
    if(!f->published) {
        ack_result r=EV(f,MB_PUBLISH,f->ticket,.input=b,.size=n);
        if(r==ACK_BUSY) return ROOM_PHONE_IO_WAIT;
        if(r!=ACK_OK) return ROOM_PHONE_IO_ERROR;
        f->published=true; return ROOM_PHONE_IO_WAIT;
    }
    ack_result r=step(f,(mb_event){.ticket=f->ticket},true,&out);
    if(r==ACK_WAIT || r==ACK_BUSY) return ROOM_PHONE_IO_WAIT;
    if(r!=ACK_OK || out.ticket!=f->ticket) return ROOM_PHONE_IO_ERROR;
    *written=n; return ROOM_PHONE_IO_OK;
}
static bool close_io(void *p) {
    fixture *f=p; mb_reply out;
    if(!f->stopping) {
        if(EV(f,MB_STOP,0,.flag=false)!=ACK_OK) return false;
        f->stopping=true;
    }
    if(!f->exited || !f->drained) return false;
    return EV(f,MB_DRAINED,0,.flag=true,.drained=true)==ACK_OK;
}
static void start(fixture *f,room_phone_channel *c,room_apsta_task *t,esp_netif_t *ap) {
    memset(f,0,sizeof *f); ack_init(&f->a); *c=(room_phone_channel){0}; *t=(room_apsta_task){0}; good=bad=0;
    t->ready=true; t->owner=&owner_id; t->session.radio.ap=ap; t->session.bridge.window=1;
    CHECK(room_phone_channel_open(c,t,(room_phone_io){f,open_io,receive_io,send_io,close_io}));
    mb_reply out; uint8_t b=42;
    CHECK(EV(f,MB_SUBMIT,0,.input=&b,.size=1,.flag=true)==ACK_OK); f->ticket=out.ticket;
    CHECK(room_phone_channel_poll(c) && t->session.transport.pending && c->reply==response);
    CHECK(room_phone_channel_poll(c) && f->published && c->offset==0 && !good);
    CHECK(EV(f,MB_TAKE_REPLY,f->ticket,.output=f->copy,.size=sizeof f->copy)==ACK_OK);
    CHECK(out.length==sizeof response && memcmp(f->copy,response,sizeof response)==0);
}
/* Worker-side simulated progress. Only complete successful output generates a receipt. */
static ack_result advance(fixture *f,size_t n,bool ok) {
    mb_reply out;
    if(!ok) return EV(f,MB_SENT,f->ticket,.flag=false);
    if(!n || n>sizeof f->copy-f->progressed) return ACK_REJECT;
    f->progressed+=n;
    if(f->progressed<sizeof f->copy) return ACK_WAIT;
    return EV(f,MB_SENT,f->ticket,.flag=true);
}
static void finish(fixture *f,room_phone_channel *c,room_apsta_task *t,bool failed) {
    CHECK(!room_phone_channel_close(c)); CHECK(t->session.transport.pending==failed && bad==0);
    f->exited=true; CHECK(!room_phone_channel_close(c));
    f->drained=true; CHECK(room_phone_channel_close(c)); CHECK(!t->session.transport.pending && !c->task && bad==(unsigned)failed);
}
int main(void) {
    fixture f; room_phone_channel c; room_apsta_task t; esp_netif_t ap={0}; mb_reply out;
    start(&f,&c,&t,&ap);
    CHECK(advance(&f,2,true)==ACK_WAIT); CHECK(room_phone_channel_poll(&c) && !good && c.offset==0);
    CHECK(advance(&f,3,true)==ACK_REJECT && f.progressed==2);
    CHECK(advance(&f,2,true)==ACK_OK);
    atomic_store(&f.a.held,true); CHECK(room_phone_channel_poll(&c) && !good); atomic_store(&f.a.held,false);
    CHECK(room_phone_channel_poll(&c) && good==1 && !c.reply && !t.session.transport.pending);
    CHECK(room_phone_channel_poll(&c) && good==1); finish(&f,&c,&t,false);
    start(&f,&c,&t,&ap); CHECK(advance(&f,1,true)==ACK_WAIT); CHECK(advance(&f,0,false)==ACK_REJECT);
    CHECK(!room_phone_channel_poll(&c) && !good); finish(&f,&c,&t,true);
    start(&f,&c,&t,&ap); f.now=2000;
    CHECK(!room_phone_channel_poll(&c) && !good); finish(&f,&c,&t,true);
    start(&f,&c,&t,&ap); f.now=1999; CHECK(EV(&f,MB_SENT,f.ticket,.flag=true)==ACK_OK); f.now=2000;
    CHECK(!room_phone_channel_poll(&c) && !good); finish(&f,&c,&t,true);
    start(&f,&c,&t,&ap); CHECK(!room_phone_channel_close(&c));
    CHECK(EV(&f,MB_SENT,f.ticket,.flag=true)==ACK_REJECT && !good); finish(&f,&c,&t,true);
    printf("offline actual channel integration: %u checks PASS\n",checks); return 0;
}
