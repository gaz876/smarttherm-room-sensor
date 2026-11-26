#ifndef ROOM_WIFI_IDF_H
#define ROOM_WIFI_IDF_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "room_wifi_enrollment.h"

/* Internal experimental adapter. Bind once, in the network task, before
 * the first STA start. No callbacks from the phone or default event loop.
 * All callback context and resources persist until controlled reboot.
 * A failed start may own resources; stop must confirm actual release. */
typedef struct {
    void *ctx;
    bool (*bind)(void *);
    bool (*ready)(void *);
    bool (*start)(void *,uint64_t deadline);
    bool (*poll)(void *);
    bool (*stop)(void *);
    bool (*load)(void *,room_enroll_profile *);
} room_wifi_setup_io;
bool room_wifi_idf_is_owner(void);
bool room_wifi_idf_bind_setup(room_wifi_setup_io);
/* Experimental station-only binding. One dedicated task owns all calls.
 * Initialize once after normal boot, with an authorized RAM-only WPA2/3 profile.
 * The default startup path remains STA-only. The optional setup binding
 * is not called by production startup. */
bool room_wifi_idf_init(const uint8_t *ssid,size_t ssid_len,const uint8_t *pass,size_t pass_len);
bool room_wifi_idf_step(bool *manual_request);
#endif
