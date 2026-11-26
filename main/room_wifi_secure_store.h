#ifndef ROOM_WIFI_SECURE_STORE_H
#define ROOM_WIFI_SECURE_STORE_H
#include "room_wifi_enrollment.h"
#include "room_setup_identity.h"
/* Single owner. No other code may initialize or open room_wifi.
 * Key ID is externally approved; never search for or create a key. */
typedef enum { ROOM_SEC_UNAVAILABLE, ROOM_SEC_MISSING, ROOM_SEC_VALID, ROOM_SEC_ERROR } room_secure_status;
bool room_secure_init(unsigned key_id);
room_secure_status room_secure_load(room_enroll_profile *out);
room_enroll_store room_secure_store(void);
/* Separate read-only namespace. No identity writes or provisioning API. */
room_secure_status room_secure_identity_load(room_setup_identity *out);
#endif
