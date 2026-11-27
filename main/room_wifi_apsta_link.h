#ifndef ROOM_WIFI_APSTA_LINK_H
#define ROOM_WIFI_APSTA_LINK_H
#include "room_wifi_idf.h"
#include "room_apsta_task.h"
#include "room_phone_channel.h"

/* Trusted local provider only; NOT a phone/network API. No default provider,
 * no startup call, no identity generation. prepare supplies one ephemeral AP
 * configuration and inline Security2 identity from trusted storage.
 * SDK context copies identity; link wipes temporary window after start.
 * No packet/listener API is exposed by this experimental binding. */
typedef struct {
    uint8_t ssid[32], key[63];
    size_t ssid_len, key_len;
    char salt[ROOM_SEC2_SALT_MAX], verifier[ROOM_SEC2_VERIFIER_SIZE];
    uint16_t salt_len, verifier_len;
    uint32_t sdk_id;
} room_wifi_apsta_window;
typedef struct {
    void *ctx;
    bool (*ready)(void *);
    bool (*prepare)(void *,room_wifi_apsta_window *);
} room_wifi_apsta_provider;
bool room_wifi_apsta_link_bind(room_wifi_apsta_provider);
/* Optional backend attachment before the first owner step. No default
 * backend and no public network endpoint in this candidate. */
bool room_wifi_apsta_link_attach_io(room_phone_io);
/* Local timed backend, receives the original owner cleanup deadline. */
bool room_wifi_apsta_link_attach_io_until(room_phone_io,room_phone_timed_close);
/* Optional first-boot candidate. Protected store must already be initialized;
 * a missing record is checked independently of the caller. No default call. */
bool room_wifi_apsta_link_init_missing(room_wifi_apsta_provider);
#endif
