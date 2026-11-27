#ifndef ROOM_APSTA_TASK_H
#define ROOM_APSTA_TASK_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct { unsigned unused; } esp_netif_t;
typedef struct { esp_netif_t *ap; } room_apsta_owner;
typedef struct {
    struct { room_apsta_owner radio; struct { uint64_t window; } bridge;
        struct { bool pending,dispatching; } transport; } session;
    void *owner;
    unsigned pending;
    bool ready,failed;
} room_apsta_task;
void *xTaskGetCurrentTaskHandle(void);
bool room_apsta_ready(room_apsta_owner *,uint64_t);
void enroll_wipe(void *,size_t);
bool room_apsta_task_poll(room_apsta_task *);
bool room_apsta_task_packet(room_apsta_task *,bool,const uint8_t *,size_t,const uint8_t **,size_t *);
bool room_apsta_task_sent(room_apsta_task *,bool);
#endif
