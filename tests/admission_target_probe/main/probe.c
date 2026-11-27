#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "room_admission.h"
#include "room_sdk_stop.h"
#include "esp_timer.h"
#include "freertos/semphr.h"

/* No server or network. Static, never-reused callback contexts outlive tasks. */
typedef struct {
    room_admission gate;
    uint64_t window;
    SemaphoreHandle_t ready, release_app, app_progress, stop_entered, release_stop, stop_progress;
    uint8_t buffer[32];
} fixture;
static fixture cases[3];
static room_sdk_stop worker;
static void check(bool ok,int line) {
    if(!ok){printf("ADMISSION_PROBE_FAIL line=%d\n",line);fflush(stdout);abort();}
}
#define CHECK(x) check((x),__LINE__)
static uint64_t now_ms(void) {
    int64_t us=esp_timer_get_time();CHECK(us>=0);return (uint64_t)us/1000u;
}
static void take(SemaphoreHandle_t s) { CHECK(xSemaphoreTake(s,pdMS_TO_TICKS(3000))==pdTRUE); }
static void give(SemaphoreHandle_t s) { CHECK(xSemaphoreGive(s)==pdTRUE); }
static void app_task(void *arg) {
    fixture *f=arg;room_lease callback={0},buffer={0};
    CHECK(room_admission_enter(&f->gate,f->window,&callback));
    CHECK(room_admission_enter(&f->gate,f->window,&buffer));
    memset(f->buffer,0x5a,sizeof(f->buffer));give(f->ready);
    take(f->release_app);CHECK(room_admission_leave(&callback));
    CHECK(!room_admission_leave(&callback));give(f->app_progress);
    take(f->release_app);
    volatile uint8_t *p=f->buffer;for(size_t i=0;i<sizeof(f->buffer);++i)p[i]=0;
    CHECK(room_admission_leave(&buffer));give(f->app_progress);
    vTaskDelete(NULL);
}
static esp_err_t synthetic_stop(httpd_handle_t handle) {
    fixture *f=handle;give(f->stop_entered);take(f->release_stop);
    give(f->stop_progress);return ESP_OK;
}
static void release_app(fixture *f) { give(f->release_app);take(f->app_progress); }
static void release_stop(fixture *f) { give(f->release_stop);take(f->stop_progress); }
static void wait_worker_return(unsigned pending) {
    uint64_t end=now_ms()+2000u;
    while(atomic_load_explicit(&worker.phase,memory_order_acquire)==pending){CHECK(now_ms()<end);vTaskDelay(1);}
}
static room_sdk_stop_result admission_probe_poll(fixture *f) {
    return room_sdk_stop_poll(&worker,f->window,room_admission_drained(&f->gate,f->window));
}
static void scenario(fixture *f,uint64_t window,bool app_first,bool late) {
    f->window=window;CHECK(room_admission_init(&f->gate,window));
    f->ready=xSemaphoreCreateBinary();f->release_app=xSemaphoreCreateBinary();f->app_progress=xSemaphoreCreateBinary();
    f->stop_entered=xSemaphoreCreateBinary();f->release_stop=xSemaphoreCreateBinary();f->stop_progress=xSemaphoreCreateBinary();
    CHECK(f->ready&&f->release_app&&f->app_progress&&f->stop_entered&&f->release_stop&&f->stop_progress);
    CHECK(xTaskCreate(app_task,"admission_app",4096,f,3,NULL)==pdPASS);take(f->ready);
    uint64_t deadline=now_ms()+(late?1000u:5000u);
    CHECK(room_admission_close(&f->gate,window));CHECK(room_admission_close(&f->gate,window));
    room_lease denied={0};CHECK(!room_admission_enter(&f->gate,window,&denied));
    CHECK(!room_admission_drained(&f->gate,window));
    CHECK(room_sdk_stop_request(&worker,window,deadline,f,synthetic_stop));take(f->stop_entered);
    CHECK(admission_probe_poll(f)==ROOM_SDK_WAIT);CHECK(!room_sdk_stop_request(&worker,window,deadline,f,synthetic_stop));
    unsigned pending=atomic_load_explicit(&worker.phase,memory_order_acquire);
    if(!app_first){release_stop(f);wait_worker_return(pending);CHECK(admission_probe_poll(f)==ROOM_SDK_WAIT);}
    release_app(f);CHECK(!room_admission_drained(&f->gate,window));CHECK(admission_probe_poll(f)==ROOM_SDK_WAIT);
    if(app_first){release_app(f);CHECK(room_admission_drained(&f->gate,window));CHECK(admission_probe_poll(f)==ROOM_SDK_WAIT);}
    if(late){while(now_ms()<deadline)vTaskDelay(1);CHECK(admission_probe_poll(f)==ROOM_SDK_LOCKED);}
    if(app_first){release_stop(f);wait_worker_return(pending);}else release_app(f);
    CHECK(room_admission_drained(&f->gate,window));
    for(size_t i=0;i<sizeof(f->buffer);++i)CHECK(f->buffer[i]==0);
    room_sdk_stop_result result=admission_probe_poll(f);uint64_t end=now_ms()+2000u;
    while(result==ROOM_SDK_WAIT){CHECK(now_ms()<end);vTaskDelay(1);result=admission_probe_poll(f);}
    CHECK(result==(late?ROOM_SDK_LOCKED:ROOM_SDK_DRAINED));
    CHECK(!room_admission_enter(&f->gate,window,&denied));
    CHECK(worker.quarantined==late);
    printf("ADMISSION_PROBE_CASE window=%llu app_first=%u late=%u handoff=%u\n",(unsigned long long)window,(unsigned)app_first,(unsigned)late,(unsigned)(result==ROOM_SDK_DRAINED));fflush(stdout);
    /* Semaphores and contexts deliberately retained until test boot ends. */
}
void app_main(void) {
    CHECK(room_sdk_stop_bind(&worker));
    scenario(&cases[0],1,false,false);scenario(&cases[1],2,true,false);scenario(&cases[2],3,true,true);
    puts("ADMISSION_PROBE_PASS");fflush(stdout);
}
