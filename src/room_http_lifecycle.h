#ifndef ROOM_HTTP_LIFECYCLE_H
#define ROOM_HTTP_LIFECYCLE_H
#include "room_admission.h"
#include "room_sdk_stop.h"
#include "esp_timer.h"

/* Inactive single-window adapter. Boot-lifetime storage; never reuse or move.
 * Prepare before registering callbacks. All application accesses to SDK
 * requests, including async completion, and borrowed buffers need a lease.
 * Denied callbacks must return without touching application resources.
 * Owner alone calls prepare/adopt/close. Reservation is enforced by the worker.
 * Publish callbacks only after adopt. An uncertain start is quarantined, not retried.
 * No SDK start, routing or restart here. */
typedef enum { HTTP_LIFE_EMPTY, HTTP_LIFE_PREPARED, HTTP_LIFE_OPEN, HTTP_LIFE_DRAIN,
    HTTP_LIFE_STOP, HTTP_LIFE_DONE, HTTP_LIFE_CANCELLED, HTTP_LIFE_LOCKED } room_http_phase;
typedef struct {
    room_admission gate;
    room_sdk_stop *worker;
    TaskHandle_t owner;
    httpd_handle_t handle;
    room_sdk_stop_fn stop;
    uint64_t window, deadline, last;
    room_http_phase phase;
} room_http_lifecycle;

static inline bool room_http_prepare(room_http_lifecycle *s,room_sdk_stop *worker,uint64_t window) {
    if(!s||s->phase!=HTTP_LIFE_EMPTY||!room_sdk_stop_reserve(worker,s,window))return false;
    s->worker=worker;s->owner=worker->owner;s->window=window;s->phase=HTTP_LIFE_PREPARED;return true;
}
static inline bool room_http_adopt(room_http_lifecycle *s,httpd_handle_t handle,room_sdk_stop_fn stop) {
    if(!s||s->phase!=HTTP_LIFE_PREPARED||s->owner!=xTaskGetCurrentTaskHandle()||
       s->worker->reservation!=s||s->worker->reserved_window!=s->window||
       s->worker->quarantined||!handle||!stop)return false;
    if(!room_admission_init(&s->gate,s->window))return false;
    s->handle=handle;s->stop=stop;s->phase=HTTP_LIFE_OPEN;return true;
}
/* Explicit owner assertion: start never produced resources or callbacks. */
static inline bool room_http_cancel_unstarted(room_http_lifecycle *s) {
    if(!s||s->phase!=HTTP_LIFE_PREPARED||s->owner!=xTaskGetCurrentTaskHandle()||
       !room_sdk_stop_cancel_unstarted(s->worker,s,s->window))return false;
    s->phase=HTTP_LIFE_CANCELLED;return true;
}
static inline bool room_http_begin(room_http_lifecycle *s,room_sdk_stop *worker,
    uint64_t window,httpd_handle_t handle,room_sdk_stop_fn stop) {
    if(!handle||!stop||!room_http_prepare(s,worker,window))return false;
    return room_http_adopt(s,handle,stop);
}
/* Callback reads only immutable published fields and synchronized gate. */
static inline bool room_http_enter(room_http_lifecycle *s,uint64_t window,room_lease *lease) {
    return s&&room_admission_enter(&s->gate,window,lease);
}
static inline room_sdk_stop_result room_http_lock(room_http_lifecycle *s) {
    if(s->worker->reservation==s)s->worker->quarantined=true;
    s->phase=HTTP_LIFE_LOCKED;return ROOM_SDK_LOCKED;
}
static inline room_sdk_stop_result room_http_close(room_http_lifecycle *s,uint64_t window,uint64_t deadline) {
    if(!s||!s->owner||s->owner!=xTaskGetCurrentTaskHandle()||
       !window||window!=s->window)return ROOM_SDK_LOCKED;
    if(s->phase==HTTP_LIFE_LOCKED||s->phase==HTTP_LIFE_CANCELLED||s->phase==HTTP_LIFE_EMPTY)return ROOM_SDK_LOCKED;
    if(s->phase==HTTP_LIFE_PREPARED)return room_http_lock(s);
    if(s->phase==HTTP_LIFE_OPEN){
        if(!room_admission_close(&s->gate,window))return room_http_lock(s);
        s->deadline=deadline;s->phase=HTTP_LIFE_DRAIN;
    }
    /* An obsolete completed context must not quarantine a reused worker. */
    if(s->phase==HTTP_LIFE_DONE)return deadline==s->deadline?ROOM_SDK_DRAINED:ROOM_SDK_LOCKED;
    if(deadline!=s->deadline)return room_http_lock(s);
    int64_t us=esp_timer_get_time();
    if(us<0)return room_http_lock(s);
    uint64_t now=(uint64_t)us/1000u;
    if(now<s->last||now>=deadline||deadline-now>5000u)return room_http_lock(s);
    s->last=now;
    /* Do not stop the SDK while application async requests may still use it.
     * Synchronous handler return is subsequently joined by the SDK stop. */
    if(s->phase==HTTP_LIFE_DRAIN){
        if(!room_admission_drained(&s->gate,window))return ROOM_SDK_WAIT;
        if(!room_sdk_stop_request_for(s->worker,s,window,deadline,s->handle,s->stop))return room_http_lock(s);
        s->handle=NULL;s->stop=NULL;s->phase=HTTP_LIFE_STOP;
    }
    room_sdk_stop_result rc=room_sdk_stop_poll_for(s->worker,s,window,true);
    if(rc==ROOM_SDK_LOCKED)return room_http_lock(s);
    if(rc==ROOM_SDK_DRAINED)s->phase=HTTP_LIFE_DONE;
    return rc;
}
#endif
