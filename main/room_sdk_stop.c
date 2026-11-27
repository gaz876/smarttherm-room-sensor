#include "room_sdk_stop.h"
#include "esp_timer.h"
#include <stddef.h>

enum { STOP_IDLE, STOP_PENDING, STOP_RETURNED, STOP_FAILED };
static bool owned(const room_sdk_stop *s) {
    return s && s->owner && s->worker && s->owner==xTaskGetCurrentTaskHandle();
}
static bool sample(room_sdk_stop *s,uint64_t *ms) {
    int64_t us=esp_timer_get_time();
    if(us<0) return false;
    *ms=(uint64_t)us/1000u;
    if(*ms<s->last_ms) return false;
    s->last_ms=*ms; return true;
}
static void stop_worker(void *arg) {
    room_sdk_stop *s=arg;
    for(;;) {
        (void)ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
        if(atomic_load_explicit(&s->phase,memory_order_acquire)!=STOP_PENDING) continue;
        esp_err_t rc=s->stop(s->handle);
        s->handle=NULL; s->stop=NULL;
        /* No request data touched after publication until the next notify. */
        atomic_store_explicit(&s->phase,rc==ESP_OK?STOP_RETURNED:STOP_FAILED,memory_order_release);
    }
}
bool room_sdk_stop_bind(room_sdk_stop *s) {
    if(!s || s->owner || s->worker || s->quarantined) return false;
    s->owner=xTaskGetCurrentTaskHandle(); atomic_init(&s->phase,STOP_IDLE);
    if(!s->owner || xTaskCreate(stop_worker,"room_stop",4096,s,4,&s->worker)!=pdPASS) {
        s->quarantined=true; return false;
    }
    return true;
}
bool room_sdk_stop_request(room_sdk_stop *s,uint64_t window,uint64_t deadline_ms,httpd_handle_t handle,room_sdk_stop_fn stop) {
    if(!owned(s) || s->quarantined || !window || window<=s->window || !handle || !stop ||
        atomic_load_explicit(&s->phase,memory_order_acquire)!=STOP_IDLE) return false;
    uint64_t now;
    if(!sample(s,&now) || deadline_ms<=now || deadline_ms-now>5000u) {
        s->quarantined=true; return false;
    }
    s->window=window; s->deadline=deadline_ms; s->handle=handle; s->stop=stop;
    atomic_store_explicit(&s->phase,STOP_PENDING,memory_order_release);
    xTaskNotifyGive(s->worker);
    return true;
}
room_sdk_stop_result room_sdk_stop_poll(room_sdk_stop *s,uint64_t window,bool application_drained) {
    if(!owned(s) || s->quarantined || !window || window!=s->window) return ROOM_SDK_LOCKED;
    unsigned phase=atomic_load_explicit(&s->phase,memory_order_acquire);
    if(phase==STOP_IDLE) return ROOM_SDK_DRAINED; /* already consumed */
    uint64_t now;
    if(!sample(s,&now) || now>=s->deadline || phase==STOP_FAILED) {
        s->quarantined=true; return ROOM_SDK_LOCKED;
    }
    if(phase!=STOP_RETURNED || !application_drained) return ROOM_SDK_WAIT;
    atomic_store_explicit(&s->phase,STOP_IDLE,memory_order_release);
    return ROOM_SDK_DRAINED;
}
