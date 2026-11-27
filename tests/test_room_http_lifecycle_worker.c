#include <stdlib.h>
#include <stdio.h>
#include "room_http_lifecycle.h"
#include "freertos/semphr.h"

#define CHECK(x) do { if(!(x)){fprintf(stderr,"HTTP_WORKER_FAIL line=%d\n",__LINE__);abort();} } while(0)

/* Actual repository worker; mock RTOS and synthetic stop, no HTTP listener. */
typedef struct {
    room_http_lifecycle life;
    room_sdk_stop worker;
    SemaphoreHandle_t ready, release, released, entered, finish;
    atomic_uint calls;
    bool fail;
    uint64_t window;
} fixture;
static fixture cases[3];
static room_http_lifecycle next[3],cancelled,partial,uncertain,blocked;
static room_sdk_stop startup_worker,uncertain_worker;
static atomic_uint partial_calls;
static uint64_t now(void){int64_t t=esp_timer_get_time();CHECK(t>=0);return (uint64_t)t/1000u;}
static void give(SemaphoreHandle_t s){CHECK(xSemaphoreGive(s)==pdTRUE);}
static void take(SemaphoreHandle_t s){CHECK(xSemaphoreTake(s,pdMS_TO_TICKS(3000))==pdTRUE);}
static void app(void *ctx){
    fixture *f=ctx;room_lease callback={0},buffer={0};
    CHECK(room_http_enter(&f->life,f->window,&callback));
    CHECK(room_http_enter(&f->life,f->window,&buffer));
    give(f->ready);take(f->release);CHECK(room_admission_leave(&callback));give(f->released);
    take(f->release);CHECK(room_admission_leave(&buffer));give(f->released);vTaskDelete(NULL);
}
static esp_err_t synthetic_stop(httpd_handle_t h){
    fixture *f=h;CHECK(room_admission_drained(&f->life.gate,f->window));
    atomic_fetch_add(&f->calls,1);give(f->entered);take(f->finish);return f->fail?-1:ESP_OK;
}
static void scenario(fixture *f,unsigned w,bool fail,bool late){
    f->window=w;f->fail=fail;atomic_init(&f->calls,0);
    f->ready=xSemaphoreCreateBinary();f->release=xSemaphoreCreateBinary();f->released=xSemaphoreCreateBinary();
    f->entered=xSemaphoreCreateBinary();f->finish=xSemaphoreCreateBinary();
    CHECK(room_sdk_stop_bind(&f->worker));CHECK(room_http_begin(&f->life,&f->worker,w,f,synthetic_stop));
    CHECK(!room_http_prepare(&next[w-1],&f->worker,w+10));
    CHECK(!room_sdk_stop_request(&f->worker,w+10,now()+1000,f,synthetic_stop));
    CHECK(room_sdk_stop_poll(&f->worker,w,true)==ROOM_SDK_LOCKED);
    CHECK(!room_sdk_stop_request_for(&f->worker,&next[w-1],w,now()+1000,f,synthetic_stop));
    CHECK(room_sdk_stop_poll_for(&f->worker,&next[w-1],w,true)==ROOM_SDK_LOCKED);
    CHECK(!room_sdk_stop_cancel_unstarted(&f->worker,&next[w-1],w));
    CHECK(xTaskCreate(app,"app",4096,f,3,NULL)==pdPASS);take(f->ready);
    uint64_t deadline=now()+(late?1000u:5000u);
    CHECK(room_http_close(&f->life,w,deadline)==ROOM_SDK_WAIT);CHECK(!atomic_load(&f->calls));
    room_lease denied={0};CHECK(!room_http_enter(&f->life,w,&denied));
    give(f->release);take(f->released);
    CHECK(room_http_close(&f->life,w,deadline)==ROOM_SDK_WAIT);CHECK(!atomic_load(&f->calls));
    give(f->release);take(f->released);
    CHECK(room_http_close(&f->life,w,deadline)==ROOM_SDK_WAIT);take(f->entered);
    CHECK(atomic_load(&f->calls)==1);CHECK(!room_http_prepare(&next[w-1],&f->worker,w+10));CHECK(room_http_close(&f->life,w,deadline)==ROOM_SDK_WAIT);
    if(late){while(now()<deadline)vTaskDelay(1);CHECK(room_http_close(&f->life,w,deadline)==ROOM_SDK_LOCKED);}
    unsigned pending=atomic_load_explicit(&f->worker.phase,memory_order_acquire);give(f->finish);
    uint64_t end=now()+2000;while(atomic_load_explicit(&f->worker.phase,memory_order_acquire)==pending){CHECK(now()<end);vTaskDelay(1);}
    room_sdk_stop_result rc=room_http_close(&f->life,w,deadline);
    CHECK(rc==((fail||late)?ROOM_SDK_LOCKED:ROOM_SDK_DRAINED));
    CHECK(f->worker.quarantined==(fail||late));CHECK(atomic_load(&f->calls)==1);
    CHECK(room_http_close(&f->life,w,deadline)==rc);
    if(fail||late){CHECK(f->worker.reservation==&f->life);CHECK(!room_http_prepare(&next[w-1],&f->worker,w+10));}
    else{
        CHECK(f->worker.reservation==NULL);CHECK(room_http_prepare(&next[w-1],&f->worker,w+10));
        CHECK(room_http_close(&f->life,w,deadline+1)==ROOM_SDK_LOCKED&&!f->worker.quarantined);
        CHECK(room_http_cancel_unstarted(&next[w-1]));
    }
    printf("HTTP_WORKER_CASE %u result=%d\n",w,(int)rc);
}
static esp_err_t partial_stop(httpd_handle_t h){CHECK(h==&partial);CHECK(room_admission_drained(&partial.gate,2));atomic_fetch_add(&partial_calls,1);return ESP_OK;}
static void startup_cases(void){
    room_lease lease={0};
    CHECK(room_sdk_stop_bind(&startup_worker));
    CHECK(room_http_prepare(&cancelled,&startup_worker,1));
    CHECK(!room_http_enter(&cancelled,1,&lease));
    CHECK(!room_http_adopt(&cancelled,NULL,partial_stop));
    CHECK(room_http_cancel_unstarted(&cancelled));
    CHECK(room_sdk_stop_poll(&startup_worker,1,true)==ROOM_SDK_LOCKED);
    CHECK(!room_http_prepare(&partial,&startup_worker,1));
    CHECK(room_http_prepare(&partial,&startup_worker,2));
    CHECK(room_http_adopt(&partial,&partial,partial_stop));
    CHECK(!room_http_cancel_unstarted(&partial));
    /* Simulate a registration failure after one callback acquired a lease. */
    CHECK(room_http_enter(&partial,2,&lease));
    uint64_t deadline=now()+5000;
    CHECK(room_http_close(&partial,2,deadline)==ROOM_SDK_WAIT);
    CHECK(!atomic_load(&partial_calls));CHECK(room_admission_leave(&lease));
    room_sdk_stop_result rc=room_http_close(&partial,2,deadline);
    while(rc==ROOM_SDK_WAIT){CHECK(now()<deadline);vTaskDelay(1);rc=room_http_close(&partial,2,deadline);}
    CHECK(rc==ROOM_SDK_DRAINED&&atomic_load(&partial_calls)==1);
    CHECK(room_sdk_stop_bind(&uncertain_worker));
    CHECK(room_http_prepare(&uncertain,&uncertain_worker,1));
    CHECK(room_http_close(&uncertain,1,now()+5000)==ROOM_SDK_LOCKED);
    CHECK(uncertain_worker.quarantined&&uncertain_worker.reservation==&uncertain);
    CHECK(!room_http_cancel_unstarted(&uncertain));
    CHECK(!room_http_prepare(&blocked,&uncertain_worker,2));
    puts("HTTP_STARTUP_PASS");
}
void app_main(void){startup_cases();scenario(&cases[0],1,false,false);scenario(&cases[1],2,true,false);scenario(&cases[2],3,false,true);puts("HTTP_WORKER_PASS");fflush(stdout);}
