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
/* Local owner callbacks only. Clock must be monotonic; false means invalid.
 * Timed backend must close admission and prove complete drain by the supplied
 * absolute deadline. It must not restart an inner budget. */
typedef bool (*room_phone_clock)(void *,uint64_t *);
typedef bool (*room_phone_timed_close)(void *,uint64_t,uint64_t);
typedef struct {
    room_phone_io io;
    room_apsta_task *task;
    uint64_t window;
    const uint8_t *reply;
    size_t length,offset;
    uint8_t packet[ROOM_PHONE_PACKET_MAX];
    bool attempted,stopping,backend_drained;
    bool timed,locked,legacy_close;
    uint64_t cleanup_deadline,cleanup_last;
    room_phone_clock clock;
    void *clock_ctx;
    room_phone_timed_close timed_close;
} room_phone_channel;
bool room_phone_channel_open(room_phone_channel *,room_apsta_task *,room_phone_io);
bool room_phone_channel_poll(room_phone_channel *);
bool room_phone_channel_close(room_phone_channel *);
/* Optional experimental absolute deadline path. A timed attempt can never
 * fall back to legacy close; preserve resources in quarantine on expiry. */
bool room_phone_channel_close_until(room_phone_channel *,uint64_t,room_phone_clock,void *,room_phone_timed_close);
#endif
