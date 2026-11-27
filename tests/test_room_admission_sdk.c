#define _POSIX_C_SOURCE 200809L
#include "room_admission.h"
#include "room_drain.h"
#include "room_phone_channel.h"
#include "room_sdk_stop.h"
#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <sched.h>
#include <stdio.h>
static unsigned checks;
#define CHECK(x) do { ++checks; assert(x); } while(0)
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
static void wait_sem(sem_t *s) { int rc;do{rc=sem_wait(s);}while(rc&&errno==EINTR);assert(!rc); }
typedef struct { room_admission gate; room_drain drain; room_sdk_stop sdk;sem_t ready,release,done;uint8_t buffer[32];uint64_t outer_deadline;bool error,partial;unsigned requests,calls; } fixture;
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
    if(f->drain.state==DRAIN_LOCKED)return false;
    if(f->drain.state==DRAIN_IDLE){
        CHECK(room_admission_close(&f->gate,7));++f->requests;
        if(!room_drain_begin(&f->drain,7,now,f->outer_deadline)||!room_sdk_stop_request(&f->sdk,7,f->outer_deadline,f,sdk_stop)){f->drain.state=DRAIN_LOCKED;return false;}
    }
    bool drained=room_admission_drained(&f->gate,7);
    room_sdk_stop_result s=room_sdk_stop_poll(&f->sdk,7,drained);
    return room_drain_step(&f->drain,7,now,s==ROOM_SDK_DRAINED,s==ROOM_SDK_LOCKED,true,drained,drained)==DRAIN_DONE;
}
#ifdef ROOM_ADMISSION_TIMED_TEST
static bool channel_clock(void *v,uint64_t *now) {
    (void)v;int64_t us=esp_timer_get_time();if(us<0)return false;*now=(uint64_t)us/1000u;return true;
}
static bool close_io_timed(void *v,uint64_t window,uint64_t deadline) {
    fixture *f=v;CHECK(window==7&&deadline==f->outer_deadline);return close_io(v);
}
#endif
static bool close_channel(room_phone_channel *c,fixture *f) {
#ifdef ROOM_ADMISSION_TIMED_TEST
    return room_phone_channel_close_until(c,f->outer_deadline,channel_clock,f,close_io_timed);
#else
    (void)f;return room_phone_channel_close(c);
#endif
}

static void *callback(void *v) {
    fixture *f=v;room_lease callback_lease={0},buffer_lease={0};
    assert(room_admission_enter(&f->gate,7,&callback_lease));assert(room_admission_enter(&f->gate,7,&buffer_lease));
    for(unsigned i=0;i<sizeof f->buffer;++i)f->buffer[i]=1;
    assert(!sem_post(&f->ready));wait_sem(&f->release);
    assert(room_admission_leave(&callback_lease));assert(!room_admission_drained(&f->gate,7));
    assert(!sem_post(&f->done));wait_sem(&f->release);
    enroll_wipe(f->buffer,sizeof f->buffer);assert(room_admission_leave(&buffer_lease));assert(!sem_post(&f->done));return NULL;
}
static void allow_stop(room_sdk_stop *s) { pthread_mutex_lock(&mu);release_stop=true;pthread_cond_broadcast(&cv);pthread_mutex_unlock(&mu);while(atomic_load_explicit(&s->phase,memory_order_acquire)==1u)sched_yield(); }
static void allow_app(fixture *f) { CHECK(!sem_post(&f->release));wait_sem(&f->done); }
static void run(bool app_first,bool partial,bool late,bool error) {
    fixture f={.outer_deadline=5100,.partial=partial,.error=error};room_phone_channel c={0};room_apsta_task t={0};esp_netif_t ap={0};pthread_t app;
    notified=quit=entered=release_stop=false;identity=&owner_id;atomic_store(&clock_us,100000);
    CHECK(room_admission_init(&f.gate,7));CHECK(!sem_init(&f.ready,0,0));CHECK(!sem_init(&f.release,0,0));CHECK(!sem_init(&f.done,0,0));
    CHECK(room_sdk_stop_bind(&f.sdk));t.owner=&owner_id;t.ready=true;t.session.bridge.window=7;t.session.radio.ap=&ap;t.session.transport.pending=true;
    CHECK(room_phone_channel_open(&c,&t,(room_phone_io){&f,open_io,receive_io,send_io,close_io})==!partial);
    CHECK(!pthread_create(&app,NULL,callback,&f));wait_sem(&f.ready);
    atomic_store(&clock_us,700000); /* outer budget began at 100, backend sees 700 */
    CHECK(!close_channel(&c,&f));pthread_mutex_lock(&mu);while(!entered)pthread_cond_wait(&cv,&mu);pthread_mutex_unlock(&mu);
    CHECK(f.requests==1&&f.calls==1&&c.attempted);CHECK(f.drain.deadline==5100&&f.sdk.deadline==5100);
#ifdef ROOM_ADMISSION_TIMED_TEST
    CHECK(c.timed&&c.cleanup_deadline==5100);CHECK(!room_phone_channel_close(&c));
#endif
    room_lease refused={0};CHECK(!room_admission_enter(&f.gate,7,&refused));CHECK(!close_channel(&c,&f));CHECK(t.session.transport.pending);
    if(!app_first)allow_stop(&f.sdk);
    allow_app(&f);CHECK(!room_admission_drained(&f.gate,7));CHECK(!close_channel(&c,&f));CHECK(t.session.transport.pending);
    if(app_first){allow_app(&f);CHECK(room_admission_drained(&f.gate,7));CHECK(!close_channel(&c,&f));}
    if(late){atomic_store(&clock_us,5100000);CHECK(!close_channel(&c,&f));}
    if(app_first)allow_stop(&f.sdk);else allow_app(&f);
    if(!late)atomic_store(&clock_us,5099000);
    CHECK(room_admission_drained(&f.gate,7));for(unsigned i=0;i<sizeof f.buffer;++i){CHECK(!f.buffer[i]);}
    CHECK(close_channel(&c,&f)==!(late||error));CHECK(f.requests==1&&f.calls==1);CHECK(c.attempted==(late||error));CHECK(t.session.transport.pending==(late||error));
    CHECK(close_channel(&c,&f)==!(late||error));CHECK(f.requests==1&&f.calls==1);
    CHECK(!pthread_join(app,NULL));pthread_mutex_lock(&mu);quit=true;pthread_cond_broadcast(&cv);pthread_mutex_unlock(&mu);CHECK(!pthread_join(worker,NULL));
    CHECK(!sem_destroy(&f.ready));CHECK(!sem_destroy(&f.release));CHECK(!sem_destroy(&f.done));
}
int main(void) { atomic_init(&clock_us,0);for(unsigned a=0;a<2;a++)for(unsigned p=0;p<2;p++)for(unsigned l=0;l<2;l++)for(unsigned e=0;e<2;e++)run(a,p,l,e);printf("admission + actual channel/worker: %u checks PASS\n",checks);return 0; }
