#define _POSIX_C_SOURCE 200809L
#include "room_sdk_stop.h"
#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static pthread_t thread;
static pthread_mutex_t mu=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cv=PTHREAD_COND_INITIALIZER;
static unsigned notify,checks,calls;
static bool quit,release_stop,entered,fail_create;
static int owner,other,worker_id,handle_id,rc;
static _Thread_local void *identity;
static atomic_int_fast64_t time_us;
static void (*entry)(void *); static void *argument;
#define CHECK(x) do { ++checks; assert(x); } while(0)
TaskHandle_t xTaskGetCurrentTaskHandle(void) {return identity;}
int64_t esp_timer_get_time(void) {return atomic_load(&time_us);}
static void *run(void *v) {(void)v;identity=&worker_id;entry(argument);return NULL;}
int xTaskCreate(void (*fn)(void *),const char *name,unsigned stack,void *arg,unsigned priority,TaskHandle_t *out) {
    (void)name;assert(stack==4096&&priority==4);if(fail_create)return 0;
    entry=fn;argument=arg;*out=&worker_id;assert(pthread_create(&thread,NULL,run,NULL)==0);return pdPASS;
}
uint32_t ulTaskNotifyTake(int clear,uint32_t wait) {
    assert(clear==pdTRUE&&wait==portMAX_DELAY);pthread_mutex_lock(&mu);
    while(!notify&&!quit)pthread_cond_wait(&cv,&mu);
    if(quit){pthread_mutex_unlock(&mu);pthread_exit(NULL);}
    unsigned n=notify;notify=0;pthread_mutex_unlock(&mu);return n;
}
void xTaskNotifyGive(TaskHandle_t h) {assert(h==&worker_id);pthread_mutex_lock(&mu);++notify;pthread_cond_broadcast(&cv);pthread_mutex_unlock(&mu);}
static esp_err_t sdk_stop(httpd_handle_t h) {
    assert(identity==&worker_id&&h==&handle_id);pthread_mutex_lock(&mu);++calls;entered=true;pthread_cond_broadcast(&cv);
    while(!release_stop)pthread_cond_wait(&cv,&mu);
    int result=rc;pthread_mutex_unlock(&mu);return result;
}
static void wait_entered(void) {pthread_mutex_lock(&mu);while(!entered)pthread_cond_wait(&cv,&mu);pthread_mutex_unlock(&mu);}
static void allow_return(room_sdk_stop *s) {pthread_mutex_lock(&mu);release_stop=true;pthread_cond_broadcast(&cv);pthread_mutex_unlock(&mu);while(atomic_load_explicit(&s->phase,memory_order_acquire)==1u)sched_yield();}
static void start_case(void) {pthread_mutex_lock(&mu);entered=release_stop=false;rc=0;pthread_mutex_unlock(&mu);}
int main(int argc,char **argv) {
    assert(argc==2);identity=&owner;atomic_init(&time_us,1000000);room_sdk_stop s={0};
    if(!strcmp(argv[1],"alloc")){fail_create=true;CHECK(!room_sdk_stop_bind(&s)&&s.quarantined);CHECK(!room_sdk_stop_bind(&s));printf("alloc: %u checks PASS\n",checks);return 0;}
    CHECK(room_sdk_stop_bind(&s));CHECK(!room_sdk_stop_bind(&s));
    CHECK(!room_sdk_stop_request(&s,0,6000,&handle_id,sdk_stop));
    identity=&other;CHECK(!room_sdk_stop_request(&s,1,6000,&handle_id,sdk_stop));identity=&owner;
    if(!strcmp(argv[1],"invalid")){CHECK(!room_sdk_stop_request(&s,1,6001,&handle_id,sdk_stop));CHECK(s.quarantined);CHECK(calls==0);}
    else {
        CHECK(room_sdk_stop_request(&s,1,6000,&handle_id,sdk_stop));wait_entered();
        CHECK(calls==1);CHECK(!room_sdk_stop_request(&s,2,6000,&handle_id,sdk_stop));
        CHECK(room_sdk_stop_poll(&s,2,true)==ROOM_SDK_LOCKED);
        identity=&other;CHECK(room_sdk_stop_poll(&s,1,true)==ROOM_SDK_LOCKED);identity=&owner;
        CHECK(room_sdk_stop_poll(&s,1,true)==ROOM_SDK_WAIT);
        if(!strcmp(argv[1],"late")){atomic_store(&time_us,6000000);CHECK(room_sdk_stop_poll(&s,1,true)==ROOM_SDK_LOCKED);allow_return(&s);CHECK(room_sdk_stop_poll(&s,1,true)==ROOM_SDK_LOCKED);}
        else if(!strcmp(argv[1],"clock")){atomic_store(&time_us,999000);CHECK(room_sdk_stop_poll(&s,1,true)==ROOM_SDK_LOCKED);allow_return(&s);CHECK(room_sdk_stop_poll(&s,1,true)==ROOM_SDK_LOCKED);}
        else if(!strcmp(argv[1],"error")){pthread_mutex_lock(&mu);rc=-1;pthread_mutex_unlock(&mu);allow_return(&s);CHECK(room_sdk_stop_poll(&s,1,true)==ROOM_SDK_LOCKED);}
        else {assert(!strcmp(argv[1],"normal"));allow_return(&s);CHECK(room_sdk_stop_poll(&s,1,false)==ROOM_SDK_WAIT);CHECK(room_sdk_stop_poll(&s,1,true)==ROOM_SDK_DRAINED);CHECK(room_sdk_stop_poll(&s,1,false)==ROOM_SDK_DRAINED);CHECK(!room_sdk_stop_request(&s,1,6000,&handle_id,sdk_stop));start_case();CHECK(room_sdk_stop_request(&s,2,6000,&handle_id,sdk_stop));wait_entered();CHECK(calls==2);allow_return(&s);CHECK(room_sdk_stop_poll(&s,2,true)==ROOM_SDK_DRAINED);}
        if(s.quarantined)CHECK(!room_sdk_stop_request(&s,2,6000,&handle_id,sdk_stop));
    }
    pthread_mutex_lock(&mu);quit=true;pthread_cond_broadcast(&cv);pthread_mutex_unlock(&mu);CHECK(pthread_join(thread,NULL)==0);
    printf("%s: %u checks PASS\n",argv[1],checks);return 0;
}
