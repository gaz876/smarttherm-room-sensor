#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "room_http_async.h"
#include "freertos/semphr.h"

typedef struct {
    room_http_lifecycle life;
    room_sdk_stop worker;
    room_http_async async;
    httpd_req_t original;
    SemaphoreHandle_t ready, completing, proceed, done;
    atomic_uint stops, completes;
    bool fail,late;
    uint64_t window;
} fixture;
static fixture cases[3];
static void take(SemaphoreHandle_t s){assert(xSemaphoreTake(s,pdMS_TO_TICKS(3000))==pdTRUE);}
static void give(SemaphoreHandle_t s){assert(xSemaphoreGive(s)==pdTRUE);}
static uint64_t now(void){int64_t us=esp_timer_get_time();assert(us>=0);return (uint64_t)us/1000u;}
esp_err_t httpd_req_async_handler_begin(httpd_req_t *r,httpd_req_t **out){
    *out=malloc(sizeof **out);assert(*out);**out=*r;return ESP_OK;
}
esp_err_t httpd_req_async_handler_complete(httpd_req_t *r){
    fixture *f=r->handle;assert(r!=&f->original);atomic_fetch_add(&f->completes,1);
    free(r); /* SDK can still need the server after the request copy is freed. */
    give(f->completing);take(f->proceed);return f->fail?-1:ESP_OK;
}
static esp_err_t stop_mock(httpd_handle_t h){
    fixture *f=h;assert(room_admission_drained(&f->life.gate,f->window));
    assert(atomic_load(&f->completes)==1);atomic_fetch_add(&f->stops,1);return ESP_OK;
}
static void callback(void *ctx){
    fixture *f=ctx;room_lease l={0};
    assert(room_http_enter(&f->life,f->window,&l));
    assert(room_http_async_begin(&f->async,&f->life,f->window,&l,&f->original));
    assert(room_admission_leave(&l));give(f->ready);
    assert(room_http_async_complete(&f->async)==!f->fail);
    assert(!room_http_async_complete(&f->async));give(f->done);vTaskDelete(NULL);
}
static void scenario(fixture *f,unsigned w,bool fail,bool late){
    f->window=w;f->fail=fail;f->late=late;f->original.handle=f;
    f->ready=xSemaphoreCreateBinary();f->completing=xSemaphoreCreateBinary();
    f->proceed=xSemaphoreCreateBinary();f->done=xSemaphoreCreateBinary();
    assert(f->ready&&f->completing&&f->proceed&&f->done);
    assert(room_sdk_stop_bind(&f->worker));assert(room_http_begin(&f->life,&f->worker,w,f,stop_mock));
    assert(xTaskCreate(callback,"callback",4096,f,3,NULL)==pdPASS);take(f->ready);take(f->completing);
    uint64_t deadline=now()+((fail||late)?300u:5000u);
    assert(room_http_close(&f->life,w,deadline)==ROOM_SDK_WAIT);assert(!atomic_load(&f->stops));
    if(late){while(now()<deadline)vTaskDelay(1);assert(room_http_close(&f->life,w,deadline)==ROOM_SDK_LOCKED);}
    give(f->proceed);take(f->done);
    room_sdk_stop_result rc=room_http_close(&f->life,w,deadline);
    uint64_t limit=now()+5500;while(rc==ROOM_SDK_WAIT){assert(now()<limit);vTaskDelay(1);rc=room_http_close(&f->life,w,deadline);}
    assert(rc==((fail||late)?ROOM_SDK_LOCKED:ROOM_SDK_DRAINED));
    assert(atomic_load(&f->stops)==((fail||late)?0u:1u));assert(atomic_load(&f->completes)==1);
    assert(f->worker.quarantined==(fail||late));
    printf("HTTP_ASYNC_WORKER_CASE %u result=%d\n",w,(int)rc);
}
void app_main(void){scenario(&cases[0],1,false,false);scenario(&cases[1],2,true,false);scenario(&cases[2],3,false,true);puts("HTTP_ASYNC_WORKER_PASS");}
