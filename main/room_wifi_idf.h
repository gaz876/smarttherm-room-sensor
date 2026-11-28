#ifndef ROOM_WIFI_IDF_H
#define ROOM_WIFI_IDF_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "room_wifi_enrollment.h"
#include "room_wifi_policy.h"

/* Internal experimental adapter. Bind once, in the network task, before
 * the first STA start. No callbacks from the phone or default event loop.
 * All callback context and resources persist until controlled reboot.
 * A failed start may own resources; stop must confirm actual release. */
typedef enum { ROOM_WIFI_SETUP_NO_SAVE, ROOM_WIFI_SETUP_COMMITTED,
    ROOM_WIFI_SETUP_UNCERTAIN } room_wifi_setup_result;
typedef struct {
    void *ctx;
    bool (*bind)(void *);
    bool (*ready)(void *);
    bool (*start)(void *,uint64_t deadline,room_wifi_profile,bool physical_boot);
    bool (*poll)(void *);
    /* Report outcome only after confirmed full cleanup; never from a packet. */
    /* Absolute deadline belongs to the owner; it must never be extended. */
    bool (*stop)(void *,uint64_t deadline,room_wifi_setup_result *);
    bool (*load)(void *,room_enroll_profile *);
} room_wifi_setup_io;
bool room_wifi_idf_is_owner(void);
bool room_wifi_idf_bind_setup(room_wifi_setup_io);
/* Experimental station-only binding. One dedicated task owns all calls.
 * Initialize once after normal boot, with an authorized RAM-only WPA2/3 profile.
 * The default startup path remains STA-only. The optional setup binding
 * is not called by production startup. */
bool room_wifi_idf_init(const uint8_t *ssid,size_t ssid_len,const uint8_t *pass,size_t pass_len);
/* Internal experimental entry: only for a confirmed missing protected
 * profile. No radio start until the first owner step with a bound provider. */
bool room_wifi_idf_init_missing(void);
bool room_wifi_idf_step(bool *manual_request);
#endif
