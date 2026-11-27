#define _POSIX_C_SOURCE 200809L
#include "room_drain.h"
#include "room_phone_channel.h"
#include "room_sdk_stop.h"
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <sched.h>
#include <stdio.h>
static unsigned checks;
#define CHECK(x) do { ++checks; assert(x); } while (0)
static int owner_id,worker_id;
static _Thread_local void *identity;
static pthread_t worker;
static pthread_mutex_t mu=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cv=PTHREAD_COND_INITIALIZER;
static bool notified,quit,entered,release_stop;
static void (*entry)(void *);static void *arg;
static atomic_int_fast64_t clock_us;
void *xTaskGetCurrentTaskHandle(void) { return identity; }
int64_t esp_timer_get_time(void) { return atomic_load(&clock_us); }
static void *worker_main(void *v) { (void)v;identity=&worker_id;entry(arg);return NULL; }
int xTaskCreate(void (*fn)(void *),const char *name,unsigned stack,void *ctx,unsigned priority,TaskHandle_t *out) {
    (void)name;(void)stack;(void)priority;entry=fn;arg=ctx;*out=&worker_id;return pthread_create(&worker,NULL,worker_main,NULL)==0?pdPASS:0;
}
uint32_t ulTaskNotifyTake(int clear,uint32_t wait) {
    assert(clear==pdTRUE&&wait==portMAX_DELAY);pthread_mutex_lock(&mu);
    while(!notified&&!quit)pthread_cond_wait(&cv,&mu);
    if(quit){pthread_mutex_unlock(&mu);pthread_exit(NULL);}
    notified=false;pthread_mutex_unlock(&mu);return 1;
}
void xTaskNotifyGive(TaskHandle_t h) { assert(h==&worker_id);pthread_mutex_lock(&mu);notified=true;pthread_cond_broadcast(&cv);pthread_mutex_unlock(&mu); }
bool room_apsta_ready(room_apsta_owner *r,uint64_t w) { return r->ap&&w; }
void enroll_wipe(void *p,size_t n) { volatile uint8_t *b=p;while(n--)*b++=0; }
bool room_apsta_task_poll(room_apsta_task *t) { return !t->failed; }
bool room_apsta_task_packet(room_apsta_task *t,bool h,const uint8_t *p,size_t n,const uint8_t **r,size_t *len) { (void)t;(void)h;(void)p;(void)n;(void)r;(void)len;assert(0);return false; }
bool room_apsta_task_sent(room_apsta_task *t,bool ok) { CHECK(!ok);t->session.transport.pending=false;return true; }
typedef struct { room_drain drain;room_sdk_stop sdk;atomic_bool callbacks,buffers;bool admission,error,partial;unsigned requests,calls; } fixture;
static esp_err_t sdk_stop(httpd_handle_t h) {
    fixture *f=h;assert(identity==&worker_id);pthread_mutex_lock(&mu);++f->calls;entered=true;pthread_cond_broadcast(&cv);
    while(!release_stop)pthread_cond_wait(&cv,&mu);
    pthread_mutex_unlock(&mu);return f->error?-1:ESP_OK;
}
static bool open_io(void *v,const esp_netif_t *ap,uint64_t w) { fixture *f=v;CHECK(ap&&w==7);return !f->partial; }
static room_phone_io_result receive_io(void *v,bool *h,uint8_t *p,size_t cap,size_t *n) { (void)v;(void)h;(void)p;(void)cap;(void)n;assert(0);return ROOM_PHONE_IO_ERROR; }
static room_phone_io_result send_io(void *v,const uint8_t *p,size_t n,size_t *written) { (void)v;(void)p;(void)n;(void)written;assert(0);return ROOM_PHONE_IO_ERROR; }
static bool close_io(void *v) {
    fixture *f=v;uint64_t now=(uint64_t)esp_timer_get_time()/1000u;
    if(f->drain.state==DRAIN_IDLE){f->admission=true;++f->requests;CHECK(room_drain_begin(&f->drain,7,now,5100));CHECK(room_sdk_stop_request(&f->sdk,7,f->drain.deadline,f,sdk_stop));}
    bool cb=atomic_load(&f->callbacks),buf=atomic_load(&f->buffers);
    room_sdk_stop_result s=room_sdk_stop_poll(&f->sdk,7,f->admission&&cb&&buf);
    return room_drain_step(&f->drain,7,now,s==ROOM_SDK_DRAINED,s==ROOM_SDK_LOCKED,f->admission,cb,buf)==DRAIN_DONE;
}
static void *app_done(void *v) { fixture *f=v;uint8_t b[16]={1};enroll_wipe(b,sizeof b);for(unsigned i=0;i<sizeof b;i++)assert(!b[i]);atomic_store(&f->buffers,true);atomic_store(&f->callbacks,true);return NULL; }
static void allow_stop(room_sdk_stop *s) { pthread_mutex_lock(&mu);release_stop=true;pthread_cond_broadcast(&cv);pthread_mutex_unlock(&mu);while(atomic_load_explicit(&s->phase,memory_order_acquire)==1u)sched_yield(); }
static void run(bool app_first,bool partial,bool late,bool error) {
    fixture f={.partial=partial,.error=error};room_phone_channel c={0};room_apsta_task t={0};esp_netif_t ap={0};pthread_t app;
    notified=quit=entered=release_stop=false;identity=&owner_id;atomic_store(&clock_us,100000);atomic_init(&f.callbacks,false);atomic_init(&f.buffers,false);
    CHECK(room_sdk_stop_bind(&f.sdk));t.owner=&owner_id;t.ready=true;t.session.bridge.window=7;t.session.radio.ap=&ap;
    CHECK(room_phone_channel_open(&c,&t,(room_phone_io){&f,open_io,receive_io,send_io,close_io})==!partial);
    CHECK(!room_phone_channel_close(&c));pthread_mutex_lock(&mu);while(!entered)pthread_cond_wait(&cv,&mu);pthread_mutex_unlock(&mu);
    CHECK(f.requests==1&&f.calls==1&&c.attempted);CHECK(!room_phone_channel_close(&c));
    if(app_first){CHECK(!pthread_create(&app,NULL,app_done,&f));CHECK(!pthread_join(app,NULL));}
    else allow_stop(&f.sdk);
    atomic_store(&clock_us,200000);CHECK(!room_phone_channel_close(&c));
    if(late){atomic_store(&clock_us,5100000);CHECK(!room_phone_channel_close(&c));}
    if(app_first)allow_stop(&f.sdk);else{CHECK(!pthread_create(&app,NULL,app_done,&f));CHECK(!pthread_join(app,NULL));}
    if(!late)atomic_store(&clock_us,5099000);
    CHECK(room_phone_channel_close(&c)==!(late||error));CHECK(f.requests==1&&f.calls==1);CHECK(c.attempted==(late||error));
    CHECK(room_phone_channel_close(&c)==!(late||error));CHECK(f.requests==1&&f.calls==1);
    pthread_mutex_lock(&mu);quit=true;pthread_cond_broadcast(&cv);pthread_mutex_unlock(&mu);CHECK(!pthread_join(worker,NULL));
}
int main(void) { atomic_init(&clock_us,0);for(unsigned a=0;a<2;a++)for(unsigned p=0;p<2;p++)for(unsigned l=0;l<2;l++)for(unsigned e=0;e<2;e++)run(a,p,l,e);printf("offline actual channel + SDK worker: %u checks PASS\n",checks);return 0; }
