#ifndef ROOM_WIFI_IDF_H
#define ROOM_WIFI_IDF_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
/* Experimental station-only binding. One dedicated task owns all calls.
 * Initialize once after normal boot, with an authorized RAM-only WPA2/3 profile.
 * No caller is wired in yet. No network settings persistence or AP. */
bool room_wifi_idf_init(const uint8_t *ssid,size_t ssid_len,const uint8_t *pass,size_t pass_len);
bool room_wifi_idf_step(bool *manual_request);
#endif
