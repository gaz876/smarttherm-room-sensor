#include "room_phone_channel.h"
#include "timed_completion_adapter.h"
#include <assert.h>
#include <stdio.h>

typedef struct {
    ack_adapter box;
    room_phone_channel channel;
    room_apsta_task task;
    esp_netif_t ap;
    uint64_t now, window, ticket;
    bool received,published,exited,drained,clock_ok;
    unsigned calls,good,bad;
    uint8_t reply;
} fixture;
static fixture f;
static int owner_id;
static unsigned checks;
#define CHECK(x) do { ++checks; assert(x); } while(0)
void *xTaskGetCurrentTaskHandle(void){return &owner_id;}
bool room_apsta_ready(room_apsta_owner *r,uint64_t w){return r->ap&&w;}
void enroll_wipe(void *p,size_t n){volatile uint8_t *b=p;while(n--)*b++=0;}
bool room_apsta_task_poll(room_apsta_task *t){return !t->failed;}
bool room_apsta_task_packet(room_apsta_task *t,bool h,const uint8_t *p,size_t n,const uint8_t **r,size_t *len){
    CHECK(h&&n==1&&!t->session.transport.pending);f.reply=(uint8_t)(p[0]^0x5a);
    *r=&f.reply;*len=1;t->session.transport.pending=true;return true;
}
bool room_apsta_task_sent(room_apsta_task *t,bool ok){
    CHECK(t->session.transport.pending&&!t->session.transport.dispatching);
    t->session.transport.pending=false;if(ok)++f.good;else ++f.bad;return true;
}
static bool clock_read(void *p,uint64_t *now){fixture *x=p;*now=x->now;return x->clock_ok;}
static ack_result event(mb_event e,bool receipt,mb_reply *out){e.window=f.window;return ack_step(&f.box,e,receipt,out,clock_read,&f);}
#define EV(a,t,...) event((mb_event){.action=a,.ticket=t,__VA_ARGS__},false,&out)
static bool open_io(void *p,const esp_netif_t *ap,uint64_t w){
    CHECK(p==&f&&w==f.window);mb_reply out;
    return ap&&EV(MB_OPEN,0,.flag=false)==ACK_OK&&EV(MB_STARTED,0,.flag=true)==ACK_OK;
}
static room_phone_io_result receive_io(void *p,bool *h,uint8_t *b,size_t cap,size_t *n){
    CHECK(p==&f);mb_reply out;*n=0;if(!f.ticket||f.received)return ROOM_PHONE_IO_WAIT;
    ack_result rc=EV(MB_TAKE_REQUEST,f.ticket,.output=b,.size=cap);
    if(rc==ACK_BUSY)return ROOM_PHONE_IO_WAIT;
    if(rc!=ACK_OK)return ROOM_PHONE_IO_ERROR;
    f.received=true;*h=out.handshake;*n=out.length;return ROOM_PHONE_IO_OK;
}
static room_phone_io_result send_io(void *p,const uint8_t *b,size_t n,size_t *written){
    CHECK(p==&f);mb_reply out;*written=0;
    if(!f.published){ack_result rc=EV(MB_PUBLISH,f.ticket,.input=b,.size=n);
        if(rc==ACK_BUSY)return ROOM_PHONE_IO_WAIT;
        if(rc!=ACK_OK)return ROOM_PHONE_IO_ERROR;
        f.published=true;return ROOM_PHONE_IO_WAIT;}
    ack_result rc=event((mb_event){.ticket=f.ticket},true,&out);
    if(rc==ACK_BUSY||rc==ACK_WAIT)return ROOM_PHONE_IO_WAIT;
    if(rc!=ACK_OK||out.ticket!=f.ticket)return ROOM_PHONE_IO_ERROR;
    *written=n;return ROOM_PHONE_IO_OK;
}
static bool no_legacy(void *p){(void)p;CHECK(false);return false;}
static bool close_io(void *p,uint64_t w,uint64_t deadline){
    CHECK(p==&f&&w==f.window);++f.calls;
    return ack_close_until(&f.box,w,deadline,f.exited,f.drained,clock_read,&f)==ACK_OK;
}
static bool close_at(uint64_t deadline){return room_phone_channel_close_until(&f.channel,deadline,clock_read,&f,close_io);}
static void open_window(uint64_t w){
    f.window=w;f.ticket=0;f.received=f.published=false;f.exited=f.drained=false;
    f.task.ready=true;f.task.owner=&owner_id;f.task.session.radio.ap=&f.ap;f.task.session.bridge.window=w;
    CHECK(room_phone_channel_open(&f.channel,&f.task,(room_phone_io){&f,open_io,receive_io,send_io,no_legacy}));
}
static void reset(void){memset(&f,0,sizeof f);f.clock_ok=true;ack_init_owner_budget(&f.box);open_window(1);}
static void exchange(uint8_t b){
    mb_reply out;uint8_t copy=0;f.received=f.published=false;
    CHECK(EV(MB_SUBMIT,0,.input=&b,.size=1,.flag=true)==ACK_OK);f.ticket=out.ticket;
    CHECK(room_phone_channel_poll(&f.channel)&&f.channel.reply&&f.task.session.transport.pending);
    CHECK(room_phone_channel_poll(&f.channel)&&f.published&&!f.channel.offset);
    CHECK(EV(MB_TAKE_REPLY,f.ticket,.output=&copy,.size=1)==ACK_OK&&copy==(uint8_t)(b^0x5a));
}
static void clean_close(uint64_t deadline){
    CHECK(!close_at(deadline));f.exited=true;CHECK(!close_at(deadline));f.drained=true;
    CHECK(close_at(deadline)&&!f.channel.task&&!f.task.session.transport.pending);
}
int main(void){
    mb_reply out;reset();
    for(unsigned i=1;i<=1000;i++){
        ++f.now;exchange((uint8_t)i);CHECK(f.ticket==i&&f.good==i-1);
        CHECK(EV(MB_SENT,f.ticket,.flag=true)==ACK_OK);
        CHECK(room_phone_channel_poll(&f.channel)&&f.good==i&&!f.channel.reply);
        CHECK(event((mb_event){.ticket=f.ticket},true,&out)==ACK_REJECT);
    }
    clean_close(6000);open_window(2);exchange(7);CHECK(f.ticket==1001);
    CHECK(ack_close_until(&f.box,1,6000,true,true,clock_read,&f)==ACK_REJECT);
    CHECK(EV(MB_SENT,f.ticket,.flag=true)==ACK_OK);CHECK(room_phone_channel_poll(&f.channel)&&f.good==1001);
    clean_close(6000);
    reset();exchange(9);CHECK(!close_at(5000)&&f.box.box.life.stop_deadline==5000);
    CHECK(EV(MB_DRAINED,0,.flag=true,.drained=true)==ACK_REJECT);
    CHECK(EV(MB_SENT,f.ticket,.flag=true)==ACK_REJECT&&!f.good);
    f.now=3000;f.exited=f.drained=true;CHECK(close_at(5000)&&f.bad==1);
    reset();exchange(9);f.now=2000;CHECK(!room_phone_channel_poll(&f.channel));
    CHECK(f.box.box.life.phase==W_STOP&&!f.box.box.life.stop_deadline);
    CHECK(EV(MB_DRAINED,0,.flag=true,.drained=true)==ACK_REJECT);
    CHECK(!close_at(7000)&&f.box.box.life.stop_deadline==7000);
    f.now=6999;f.exited=f.drained=true;CHECK(close_at(7000)&&f.bad==1);
    reset();exchange(9);CHECK(!close_at(5000));f.now=5000;f.exited=f.drained=true;
    unsigned calls=f.calls;CHECK(!close_at(5000)&&f.channel.locked&&f.calls==calls&&f.task.session.transport.pending);
    CHECK(ack_close_until(&f.box,1,5000,true,true,clock_read,&f)==ACK_REJECT);
    CHECK(f.box.box.life.phase==W_LOCKED);
    reset();exchange(9);CHECK(!close_at(5000));
    CHECK(ack_close_until(&f.box,1,4999,true,true,clock_read,&f)==ACK_REJECT&&f.box.box.life.phase==W_LOCKED);
    CHECK(!close_at(5000)&&f.task.session.transport.pending);
    reset();exchange(9);f.now=10;CHECK(!close_at(5010));f.now=9;
    CHECK(ack_close_until(&f.box,1,5010,true,true,clock_read,&f)==ACK_REJECT&&f.box.box.life.phase==W_LOCKED);
    reset();exchange(9);f.clock_ok=false;
    CHECK(ack_close_until(&f.box,1,5000,true,true,clock_read,&f)==ACK_REJECT&&f.box.box.life.phase==W_LOCKED);
    reset();exchange(9);atomic_store(&f.box.held,true);CHECK(!close_at(5000));atomic_store(&f.box.held,false);
    CHECK(!room_phone_channel_poll(&f.channel));f.exited=f.drained=true;CHECK(close_at(5000)&&f.bad==1);
    reset();exchange(9);CHECK(EV(MB_SENT,f.ticket,.flag=false)==ACK_REJECT);
    CHECK(!room_phone_channel_poll(&f.channel));clean_close(5000);CHECK(f.bad==1&&!f.good);
    printf("CHANNEL_OWNER_BUDGET_PASS checks=%u exchanges=1001\n",checks);return 0;
}
