#ifndef ROOM_SDK_STOP_H
#define ROOM_SDK_STOP_H
#include <stdbool.h>
#include <stdint.h>
#include <stdatomic.h>
#include "esp_http_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Experimental, uncalled. One persistent context and worker per boot.
 * Zero initialize once; never move, free or share with a network callback.
 * Only the binding task may request/poll. Before request the caller must
 * close application admission and exclusively transfer the SDK handle.
 * The caller must never use that handle again, even on failure/timeout.
 * Poll's drain proof must come from actual application quiescence, not a
 * remote field, timer or SDK event alone. This is not a complete backend. */
typedef esp_err_t (*room_sdk_stop_fn)(httpd_handle_t);
typedef enum { ROOM_SDK_WAIT, ROOM_SDK_DRAINED, ROOM_SDK_LOCKED } room_sdk_stop_result;
typedef struct {
    TaskHandle_t owner, worker;
    atomic_uint phase;
    httpd_handle_t handle;
    room_sdk_stop_fn stop;
    uint64_t window, deadline, last_ms;
    bool quarantined, completed;
    /* Owner-task only; held from pre-start to successful drain or quarantine. */
    const void *reservation;
    uint64_t reserved_window;
} room_sdk_stop;
bool room_sdk_stop_bind(room_sdk_stop *);
bool room_sdk_stop_reserve(room_sdk_stop *,const void *,uint64_t);
/* Cancel only if no SDK resource or callback was ever published. */
bool room_sdk_stop_cancel_unstarted(room_sdk_stop *,const void *,uint64_t);
bool room_sdk_stop_request_for(room_sdk_stop *,const void *,uint64_t,uint64_t,httpd_handle_t,room_sdk_stop_fn);
room_sdk_stop_result room_sdk_stop_poll_for(room_sdk_stop *,const void *,uint64_t,bool);
bool room_sdk_stop_request(room_sdk_stop *,uint64_t window,uint64_t deadline_ms,httpd_handle_t,room_sdk_stop_fn);
room_sdk_stop_result room_sdk_stop_poll(room_sdk_stop *,uint64_t window,bool application_drained);
#endif
