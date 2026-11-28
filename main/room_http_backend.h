#ifndef ROOM_HTTP_BACKEND_H
#define ROOM_HTTP_BACKEND_H
#include "room_network_owner.h"
#include "room_http_lifecycle.h"
#include <stdatomic.h>

/* Offline candidate. SDK driver must establish interface and peer isolation.
 * No default driver, no listener registered by the regular startup path. */
typedef struct room_http_backend_window room_http_backend_window;
typedef enum { ROOM_HTTP_START_CLEAN_FAILURE, ROOM_HTTP_START_READY,
    ROOM_HTTP_START_UNCERTAIN } room_http_start_result;
typedef struct {
    void *ctx;
    /* No request callbacks before publish. Clean failure proves no resources. */
    room_http_start_result (*start)(void *,const esp_netif_t *,httpd_handle_t *);
    bool (*publish)(void *,httpd_handle_t,room_http_backend_window *);
    /* Success must prove all callback sources are joined and no pointers retained. */
    room_sdk_stop_fn stop;
    /* Trusted SDK-local peer check; never use client-provided generation/identity. */
    bool (*peer)(void *,const esp_netif_t *,const void *request);
} room_http_driver;
typedef struct {
    room_http_backend_window *window;
    room_lease lease;
    bool active, copied;
} room_http_exchange;
struct room_http_backend_window {
    room_http_lifecycle life;
    room_http_driver driver;
    const esp_netif_t *ap;
    atomic_uint phase;
    atomic_bool stopping, accepting;
    uint8_t bytes[ROOM_PHONE_PACKET_MAX];
    size_t length;
    bool handshake;
};
typedef struct {
    room_sdk_stop worker;
    room_http_driver driver;
    room_http_backend_window storage;
    room_http_backend_window *current;
    uint64_t last_attempt,closed_window,closed_deadline;
    uint64_t rejected_window;
    bool initialized, failed;
} room_http_backend;
/* Boot-lifetime storage, one owner. Start a new logical lifetime only after
 * a successful SDK stop (all callbacks joined), or proven unstarted cancel.
 * Driver must never retain a window pointer after successful stop. On any
 * uncertain cleanup, storage is quarantined permanently. */
bool room_http_backend_prepare(room_http_backend *,room_http_driver,room_network_setup_backend *);
bool room_http_backend_prepare_owner(room_http_backend *,room_http_driver,unsigned,room_secure_status *);
/* Callback-only interface. Exchange is zero-initialized, never copied.
 * Finish is mandatory after a successful offer, including abort/error paths. */
bool room_http_backend_offer(room_http_backend_window *,room_http_exchange *,const void *,bool,const uint8_t *,size_t);
room_phone_io_result room_http_backend_reply(room_http_exchange *,uint8_t *,size_t,size_t *);
bool room_http_backend_finish(room_http_exchange *,bool delivered);
#endif
