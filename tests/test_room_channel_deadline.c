#include "room_phone_channel.h"
#include <assert.h>
#include <stdio.h>
static unsigned checks;
#define CHECK(x) do { ++checks; assert(x); } while(0)
static int owner_id;
static void *identity=&owner_id;
void *xTaskGetCurrentTaskHandle(void) {return identity;}
bool room_apsta_ready(room_apsta_owner *r,uint64_t w) {return r->ap&&w;}
void enroll_wipe(void *p,size_t n) {volatile uint8_t *b=p;while(n--)*b++=0;}
bool room_apsta_task_poll(room_apsta_task *t) {return !t->failed;}
bool room_apsta_task_packet(room_apsta_task *t,bool h,const uint8_t *p,size_t n,const uint8_t **r,size_t *len) {(void)t;(void)h;(void)p;(void)n;(void)r;(void)len;assert(0);return false;}
static unsigned completions;
bool room_apsta_task_sent(room_apsta_task *t,bool ok) {CHECK(!ok);++completions;t->session.transport.pending=false;return true;}
typedef struct {uint64_t now,expected;unsigned calls,legacy;bool done,late,invalid,partial;} fixture;
static bool clock_ms(void *v,uint64_t *now){fixture *f=v;*now=f->now;return !f->invalid;}
static bool open_io(void *v,const esp_netif_t *ap,uint64_t w){fixture *f=v;CHECK(ap&&w==7);return !f->partial;}
static room_phone_io_result receive_io(void *v,bool *h,uint8_t *p,size_t cap,size_t *n){(void)v;(void)h;(void)p;(void)cap;(void)n;assert(0);return ROOM_PHONE_IO_ERROR;}
static room_phone_io_result send_io(void *v,const uint8_t *p,size_t n,size_t *written){(void)v;(void)p;(void)n;(void)written;assert(0);return ROOM_PHONE_IO_ERROR;}
static bool legacy(void *v){fixture *f=v;++f->legacy;return false;}
static bool close_io(void *v,uint64_t w,uint64_t deadline){fixture *f=v;CHECK(w==7&&deadline==f->expected);++f->calls;if(f->late)f->now=deadline;return f->done;}
static bool step(room_phone_channel *c,fixture *f,uint64_t deadline){return room_phone_channel_close_until(c,deadline,clock_ms,f,close_io);}
static void run(unsigned scenario,bool partial){
    fixture f={.now=700,.expected=5100,.partial=partial};room_phone_channel c={0};room_apsta_task t={0};esp_netif_t ap={0};
    t.owner=&owner_id;t.ready=true;t.session.bridge.window=7;t.session.radio.ap=&ap;t.session.transport.pending=true;completions=0;
    CHECK(room_phone_channel_open(&c,&t,(room_phone_io){&f,open_io,receive_io,send_io,legacy})==!partial);
    if(scenario==8){CHECK(!room_phone_channel_close(&c));CHECK(!step(&c,&f,5100));CHECK(c.locked&&f.legacy==1&&f.calls==0);return;}
    CHECK(!step(&c,&f,5100));CHECK(c.stopping&&c.timed&&c.cleanup_deadline==5100&&f.calls==1);
    CHECK(!room_phone_channel_close(&c));CHECK(!f.legacy);CHECK(!room_phone_channel_poll(&c));CHECK(!completions);
    f.done=true;f.now=5099;
    if(scenario==1)f.now=5100;
    if(scenario==2)f.late=true;
    if(scenario==3)f.now=699;
    if(scenario==4)f.invalid=true;
    if(scenario==5)t.session.bridge.window=8;
    if(scenario==7){t.session.transport.dispatching=true;CHECK(!step(&c,&f,5100));CHECK(c.backend_drained&&f.calls==2&&!completions);t.session.transport.dispatching=false;}
    bool ok=step(&c,&f,scenario==6?5200:5100);
    CHECK(ok==(scenario==0||scenario==7));
    if(ok){CHECK(!c.attempted&&!t.session.transport.pending&&completions==1);CHECK(step(&c,&f,5100));CHECK(f.calls==2);}
    else{CHECK(c.attempted&&c.locked&&t.session.transport.pending&&!completions);unsigned n=f.calls;f.now=5099;f.invalid=f.late=false;t.session.bridge.window=7;CHECK(!step(&c,&f,5100));CHECK(f.calls==n);}
}
int main(void){for(unsigned i=0;i<9;i++)for(unsigned p=0;p<2;p++)run(i,p);printf("channel absolute deadline: %u checks PASS\n",checks);return 0;}
