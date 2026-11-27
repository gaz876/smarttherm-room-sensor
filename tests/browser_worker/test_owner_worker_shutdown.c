#define _POSIX_C_SOURCE 200809L
#include "room_phone_channel.h"
#include "room_wifi_owner.h"
#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <stdio.h>
#include <errno.h>

/* Host-only fixture. The stop worker blocks on a synthetic SDK operation.
 * The test supervisor, not the owner, joins threads. The 5 s outer gate
 * models room_wifi_idf.c; it is not execution of that adapter. */
typedef struct {
    room_phone_channel channel;
    room_apsta_task task;
    esp_netif_t ap;
    sem_t requested_stop, entered, release_stop, release_callback;
    pthread_t worker, callback;
    atomic_bool stop_returned, buffer_wiped;
    bool worker_joined, callback_joined, fatal, requested;
    uint64_t now, deadline;
    unsigned active, starts, close_calls, stop_requests;
} fixture;
static int owner_id;
static unsigned checks;
#define CHECK(x) do { ++checks; assert(x); } while (0)
static void wait_sem(sem_t *s) { int rc; do { rc=sem_wait(s); } while(rc && errno==EINTR); assert(rc==0); }
void *xTaskGetCurrentTaskHandle(void) { return &owner_id; }
bool room_apsta_ready(room_apsta_owner *r,uint64_t w) { return r->ap && w; }
void enroll_wipe(void *p,size_t n) { volatile uint8_t *b=p; while(n--) *b++=0; }
bool room_apsta_task_poll(room_apsta_task *t) { return !t->failed; }
bool room_apsta_task_packet(room_apsta_task *t,bool h,const uint8_t *p,size_t n,const uint8_t **r,size_t *len) {
    (void)t;(void)h;(void)p;(void)n;(void)r;(void)len;assert(0);return false;
}
bool room_apsta_task_sent(room_apsta_task *t,bool ok) { (void)t;(void)ok;assert(0);return false; }
static void *stop_worker(void *v) {
    fixture *f=v; wait_sem(&f->requested_stop); assert(sem_post(&f->entered)==0); wait_sem(&f->release_stop);
    atomic_store(&f->stop_returned,true); return NULL;
}
static void *callback_worker(void *v) {
    fixture *f=v; uint8_t copy[32]={1}; wait_sem(&f->release_callback);
    enroll_wipe(copy,sizeof copy); for(size_t i=0;i<sizeof copy;++i)assert(copy[i]==0);
    atomic_store(&f->buffer_wiped,true); return NULL;
}
static bool open_io(void *v,const esp_netif_t *ap,uint64_t w) { (void)v;return ap && w; }
static room_phone_io_result receive_io(void *v,bool *h,uint8_t *p,size_t cap,size_t *n) { (void)v;(void)h;(void)p;(void)cap;*n=0;return ROOM_PHONE_IO_WAIT; }
static room_phone_io_result send_io(void *v,const uint8_t *p,size_t n,size_t *written) { (void)v;(void)p;(void)n;*written=0;return ROOM_PHONE_IO_ERROR; }
static bool close_io(void *v) {
    fixture *f=v; ++f->close_calls;
    if(!f->requested){f->requested=true;++f->stop_requests;assert(sem_post(&f->requested_stop)==0); }
    return f->worker_joined && f->callback_joined && atomic_load(&f->stop_returned) && atomic_load(&f->buffer_wiped);
}
static bool stop(void *v) {
    fixture *f=v;
    if(f->fatal)return false;
    if(!f->deadline)f->deadline=wifi_deadline(f->now,5000);
    if(f->now>=f->deadline){f->fatal=true;return false;}
    if(f->active==2 && !room_phone_channel_close(&f->channel))return false;
    f->active=0; f->deadline=0; return true;
}
static bool station(void *v,uint64_t token) { fixture *f=v; CHECK(!f->active && token && !f->channel.attempted && !f->fatal);f->active=1;++f->starts;return true; }
static bool setup(void *v,uint64_t token) {
    fixture *f=v;CHECK(!f->active && token);f->active=2;
    f->task.ready=true;f->task.owner=&owner_id;f->task.session.radio.ap=&f->ap;f->task.session.bridge.window=token;
    return room_phone_channel_open(&f->channel,&f->task,(room_phone_io){f,open_io,receive_io,send_io,close_io});
}
static void tick(fixture *f,room_wifi_owner *o,uint64_t now,bool manual,room_wifi_event e) {f->now=now;wifi_owner_step(o,now,manual,true,e);}
static void finish_worker(fixture *f) {CHECK(sem_post(&f->release_stop)==0);CHECK(pthread_join(f->worker,NULL)==0);f->worker_joined=true;}
static void finish_callback(fixture *f) {CHECK(sem_post(&f->release_callback)==0);CHECK(pthread_join(f->callback,NULL)==0);f->callback_joined=true;}
static void run(bool late,bool callback_first) {
    fixture f={0};room_wifi_owner o;
    atomic_init(&f.stop_returned,false);atomic_init(&f.buffer_wiped,false);
    CHECK(sem_init(&f.requested_stop,0,0)==0);CHECK(sem_init(&f.entered,0,0)==0);CHECK(sem_init(&f.release_stop,0,0)==0);CHECK(sem_init(&f.release_callback,0,0)==0);
    CHECK(pthread_create(&f.worker,NULL,stop_worker,&f)==0);CHECK(pthread_create(&f.callback,NULL,callback_worker,&f)==0);
    wifi_owner_init(&o,(room_wifi_transport){&f,stop,station,setup},ROOM_WIFI_PROFILE_VALID,0,true);
    tick(&f,&o,1,false,(room_wifi_event){0});CHECK(f.active==1 && f.starts==1);
    tick(&f,&o,2,true,(room_wifi_event){0});CHECK(f.active==2 && o.active && f.channel.attempted);
    tick(&f,&o,10,false,(room_wifi_event){WIFI_EVENT_SETUP_ABORTED,o.active});
    wait_sem(&f.entered);
    CHECK(f.requested && f.stop_requests==1 && f.active==2 && o.dirty && !o.active);
    for(uint64_t t=1010;t<=2010;t+=1000){tick(&f,&o,t,false,(room_wifi_event){0});CHECK(f.active==2 && f.starts==1 && !atomic_load(&f.stop_returned));}
    if(callback_first)finish_callback(&f);else finish_worker(&f);
    tick(&f,&o,3010,false,(room_wifi_event){0});CHECK(f.active==2 && f.starts==1 && o.dirty);
    if(late){tick(&f,&o,4010,false,(room_wifi_event){0});CHECK(!f.fatal && f.active==2);tick(&f,&o,5010,false,(room_wifi_event){0});CHECK(f.fatal && f.active==2 && o.dirty);}
    if(callback_first)finish_worker(&f);else finish_callback(&f);
    tick(&f,&o,late?6010:4010,false,(room_wifi_event){0});
    CHECK(f.stop_requests==1 && atomic_load(&f.stop_returned) && atomic_load(&f.buffer_wiped));
    if(late){CHECK(f.fatal && o.dirty && !o.active && f.active==2 && f.starts==1 && f.channel.attempted);}
    else{CHECK(!f.fatal && f.active==1 && f.starts==2 && !f.channel.attempted && o.active);}
    CHECK(sem_destroy(&f.requested_stop)==0);CHECK(sem_destroy(&f.entered)==0);CHECK(sem_destroy(&f.release_stop)==0);CHECK(sem_destroy(&f.release_callback)==0);
}
int main(void) {
    run(false,false);run(false,true);run(true,false);run(true,true);
    printf("offline owner/worker shutdown: 4 scenarios, %u checks PASS\n",checks);return 0;
}
