#include <assert.h>
#include <stdio.h>
#include "room_http_lifecycle.h"
#include "freertos/semphr.h"

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
static uint64_t now(void){int64_t t=esp_timer_get_time();assert(t>=0);return (uint64_t)t/1000u;}
static void give(SemaphoreHandle_t s){assert(xSemaphoreGive(s)==pdTRUE);}
static void take(SemaphoreHandle_t s){assert(xSemaphoreTake(s,3000)==pdTRUE);}
static void app(void *ctx){
    fixture *f=ctx;room_lease lease={0};assert(room_http_enter(&f->life,f->window,&lease));
    give(f->ready);take(f->release);assert(room_admission_leave(&lease));give(f->released);vTaskDelete(NULL);
}
static esp_err_t synthetic_stop(httpd_handle_t h){
    fixture *f=h;assert(room_admission_drained(&f->life.gate,f->window));
    atomic_fetch_add(&f->calls,1);give(f->entered);take(f->finish);return f->fail?-1:ESP_OK;
}
static void scenario(fixture *f,unsigned w,bool fail,bool late){
    f->window=w;f->fail=fail;atomic_init(&f->calls,0);
    f->ready=xSemaphoreCreateBinary();f->release=xSemaphoreCreateBinary();f->released=xSemaphoreCreateBinary();
    f->entered=xSemaphoreCreateBinary();f->finish=xSemaphoreCreateBinary();
    assert(room_sdk_stop_bind(&f->worker));assert(room_http_begin(&f->life,&f->worker,w,f,synthetic_stop));
    assert(xTaskCreate(app,"app",4096,f,3,NULL)==pdPASS);take(f->ready);
    uint64_t deadline=now()+(late?1000u:5000u);
    assert(room_http_close(&f->life,w,deadline)==ROOM_SDK_WAIT);assert(!atomic_load(&f->calls));
    room_lease denied={0};assert(!room_http_enter(&f->life,w,&denied));
    give(f->release);take(f->released);
    assert(room_http_close(&f->life,w,deadline)==ROOM_SDK_WAIT);take(f->entered);
    assert(atomic_load(&f->calls)==1);assert(room_http_close(&f->life,w,deadline)==ROOM_SDK_WAIT);
    if(late){while(now()<deadline)vTaskDelay(1);assert(room_http_close(&f->life,w,deadline)==ROOM_SDK_LOCKED);}
    unsigned pending=atomic_load_explicit(&f->worker.phase,memory_order_acquire);give(f->finish);
    uint64_t end=now()+2000;while(atomic_load_explicit(&f->worker.phase,memory_order_acquire)==pending){assert(now()<end);vTaskDelay(1);}
    room_sdk_stop_result rc=room_http_close(&f->life,w,deadline);
    assert(rc==((fail||late)?ROOM_SDK_LOCKED:ROOM_SDK_DRAINED));
    assert(f->worker.quarantined==(fail||late));assert(atomic_load(&f->calls)==1);
    assert(room_http_close(&f->life,w,deadline)==rc);
    printf("HTTP_WORKER_CASE %u result=%d\n",w,(int)rc);
}
void app_main(void){scenario(&cases[0],1,false,false);scenario(&cases[1],2,true,false);scenario(&cases[2],3,false,true);puts("HTTP_WORKER_PASS");}
