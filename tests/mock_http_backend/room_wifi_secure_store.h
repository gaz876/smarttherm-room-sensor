#ifndef ROOM_WIFI_SECURE_STORE_H
#define ROOM_WIFI_SECURE_STORE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct { uint8_t ssid_len, pass_len; uint8_t ssid[32], pass[63]; } room_enroll_profile;
typedef enum { ROOM_SEC_UNAVAILABLE, ROOM_SEC_MISSING, ROOM_SEC_VALID, ROOM_SEC_ERROR } room_secure_status;
bool room_secure_init(unsigned);
room_secure_status room_secure_load(room_enroll_profile *);
void enroll_wipe(void *,size_t);
#endif
