#define _POSIX_C_SOURCE 200809L
#include "room_drain.h"
#include "room_phone_channel.h"
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
static unsigned checks;
#define CHECK(x) do { ++checks; assert(x); } while (0)
static int owner_id;
void *xTaskGetCurrentTaskHandle(void) { return &owner_id; }
bool room_apsta_ready(room_apsta_owner *r,uint64_t w) { return r->ap && w; }
void enroll_wipe(void *p,size_t n) { volatile uint8_t *b=p; while(n--) *b++=0; }
bool room_apsta_task_poll(room_apsta_task *t) { return !t->failed; }
bool room_apsta_task_packet(room_apsta_task *t,bool h,const uint8_t *p,size_t n,const uint8_t **r,size_t *len) {
    (void)t;(void)h;(void)p;(void)n;(void)r;(void)len;assert(0);return false;
}
bool room_apsta_task_sent(room_apsta_task *t,bool ok) { CHECK(!ok);t->session.transport.pending=false;return true; }
typedef struct {
    room_drain drain;
    atomic_bool sdk, callbacks, buffers;
    bool admission, partial, error;
    uint64_t now, window;
    unsigned closes, requests;
} fixture;
static bool open_io(void *v,const esp_netif_t *ap,uint64_t w) { fixture *f=v;CHECK(ap && w==f->window);return !f->partial; }
static room_phone_io_result receive_io(void *v,bool *h,uint8_t *p,size_t cap,size_t *n) { (void)v;(void)h;(void)p;(void)cap;(void)n;assert(0);return ROOM_PHONE_IO_ERROR; }
static room_phone_io_result send_io(void *v,const uint8_t *p,size_t n,size_t *written) { (void)v;(void)p;(void)n;(void)written;assert(0);return ROOM_PHONE_IO_ERROR; }
static bool close_io(void *v) {
    fixture *f=v;++f->closes;
    if(f->drain.state==DRAIN_IDLE){ f->admission=true; ++f->requests; CHECK(room_drain_begin(&f->drain,f->window,f->now,5100)); }
    return room_drain_step(&f->drain,f->window,f->now,atomic_load(&f->sdk),f->error,f->admission,atomic_load(&f->callbacks),atomic_load(&f->buffers))==DRAIN_DONE;
}
static void *complete(void *v) { atomic_store((atomic_bool *)v,true);return NULL; }
static void run(unsigned first,bool partial,bool late,bool error,bool parallel) {
    fixture f={.partial=partial,.window=7,.now=100};room_phone_channel c={0};room_apsta_task t={0};esp_netif_t ap={0};
    atomic_init(&f.sdk,false);atomic_init(&f.callbacks,false);atomic_init(&f.buffers,false);
    t.owner=&owner_id;t.ready=true;t.session.bridge.window=7;t.session.radio.ap=&ap;
    CHECK(room_phone_channel_open(&c,&t,(room_phone_io){&f,open_io,receive_io,send_io,close_io})==!partial);
    CHECK(!room_phone_channel_close(&c));CHECK(f.admission && f.requests==1 && c.attempted);
    CHECK(!room_phone_channel_poll(&c));CHECK(!room_phone_channel_close(&c));CHECK(f.requests==1);
    atomic_bool *flags[3]={&f.sdk,&f.callbacks,&f.buffers};pthread_t th[3];
    if(parallel){for(unsigned i=0;i<3;i++)CHECK(!pthread_create(&th[i],NULL,complete,flags[i]));for(unsigned i=0;i<3;i++)CHECK(!pthread_join(th[i],NULL));}
    else {for(unsigned i=0;i<2;i++){unsigned k=(first+i)%3;CHECK(!pthread_create(&th[k],NULL,complete,flags[k]));CHECK(!pthread_join(th[k],NULL));f.now+=100;CHECK(!room_phone_channel_close(&c));}unsigned k=(first+2)%3;CHECK(!pthread_create(&th[k],NULL,complete,flags[k]));CHECK(!pthread_join(th[k],NULL));}
    f.now=late?5100:5099;f.error=error;
    CHECK(room_phone_channel_close(&c)==!(late||error));CHECK(f.requests==1);
    f.error=false;f.now=5101;unsigned n=f.closes;
    CHECK(room_phone_channel_close(&c)==!(late||error));CHECK(f.requests==1);
    CHECK(f.closes==n+(late||error));CHECK(c.attempted==(late||error));
}
int main(void) {
    for(unsigned i=0;i<3;i++)for(unsigned p=0;p<2;p++)for(unsigned l=0;l<2;l++)for(unsigned e=0;e<2;e++)run(i,p,l,e,false);
    run(0,false,false,false,true);run(0,true,true,false,true);
    room_drain d={0};CHECK(room_drain_begin(&d,1,100,5100));CHECK(room_drain_begin(&d,1,200,5100));CHECK(!room_drain_begin(&d,1,200,5200));CHECK(d.state==DRAIN_LOCKED);
    d=(room_drain){0};CHECK(room_drain_begin(&d,2,100,5100));CHECK(room_drain_step(&d,2,101,true,false,false,true,true)==DRAIN_WAIT);CHECK(room_drain_step(&d,1,102,true,false,true,true,true)==DRAIN_LOCKED);
    d=(room_drain){0};CHECK(room_drain_begin(&d,3,100,5100));CHECK(room_drain_step(&d,3,99,true,false,true,true,true)==DRAIN_LOCKED);
    d=(room_drain){0};CHECK(!room_drain_begin(&d,1,100,5101));CHECK(d.state==DRAIN_LOCKED);
    d=(room_drain){0};CHECK(room_drain_begin(&d,4,UINT64_MAX-10,UINT64_MAX));CHECK(room_drain_step(&d,4,UINT64_MAX,true,false,true,true,true)==DRAIN_LOCKED);
    printf("offline deadline/channel: %u checks PASS\n",checks);return 0;
}
