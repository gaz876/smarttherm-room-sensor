#ifndef ROOM_PHONE_CHANNEL_H
#define ROOM_PHONE_CHANNEL_H
#include "room_apsta_task.h"

/* Optional owner-task transport boundary. Backend must bind only the current
 * AP interface, accept one local peer, never retain any buffer pointer and
 * make each operation bounded/nonblocking. Not a socket or HTTP parser.
 * Window and peer association are backend responsibilities; never derive
 * trusted generations from request fields. Failed open may own resources. */
enum { ROOM_PHONE_PACKET_MAX=4096 };
typedef enum { ROOM_PHONE_IO_WAIT, ROOM_PHONE_IO_OK, ROOM_PHONE_IO_ERROR } room_phone_io_result;
typedef struct {
    void *ctx;
    bool (*open)(void *,const esp_netif_t *ap,uint64_t window);
    room_phone_io_result (*receive)(void *,bool *handshake,uint8_t *,size_t,size_t *);
    room_phone_io_result (*send)(void *,const uint8_t *,size_t,size_t *);
    /* True only after no input/output/backend callbacks can remain. */
    bool (*close)(void *);
} room_phone_io;
typedef struct {
    room_phone_io io;
    room_apsta_task *task;
    uint64_t window;
    const uint8_t *reply;
    size_t length,offset;
    uint8_t packet[ROOM_PHONE_PACKET_MAX];
    bool attempted,stopping;
} room_phone_channel;
bool room_phone_channel_open(room_phone_channel *,room_apsta_task *,room_phone_io);
bool room_phone_channel_poll(room_phone_channel *);
bool room_phone_channel_close(room_phone_channel *);
#endif
