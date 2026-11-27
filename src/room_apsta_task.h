#ifndef ROOM_APSTA_TASK_H
#define ROOM_APSTA_TASK_H
#include "room_apsta_events.h"
#include "freertos/task.h"

enum { ROOM_APSTA_QUEUE_DEPTH=8 };

/* Persistent candidate, called only by the exclusive network task after the
 * station adapter has fully stopped. The context embeds its own static queue;
 * it must be zero-initialized once, never moved, and live until reboot.
 * No startup invocation, listener, identity provisioning or eFuse changes. */
typedef struct {
    room_apsta_session session;
    room_apsta_events events;
    StaticQueue_t queue_control;
    room_apsta_event queue_storage[ROOM_APSTA_QUEUE_DEPTH];
    TaskHandle_t owner;
    esp_event_handler_instance_t barrier_handler;
    atomic_uint ack;
    uint32_t sequence, pending;
    uint64_t barrier_window;
    bool final_phase, pretrial_phase, ready, failed;
} room_apsta_task;
bool room_apsta_task_bind(room_apsta_task *);
bool room_apsta_task_open(room_apsta_task *, phone_profile_state, bool, bool,
    const room_enroll_store *,const uint8_t *,size_t,const uint8_t *,size_t,
    const protocomm_security2_params_t *,uint32_t);
bool room_apsta_task_poll(room_apsta_task *);
bool room_apsta_task_packet(room_apsta_task *,bool,const uint8_t *,size_t,const uint8_t **,size_t *);
bool room_apsta_task_sent(room_apsta_task *,bool);
bool room_apsta_task_close(room_apsta_task *,bool);
#endif
