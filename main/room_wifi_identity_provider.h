#ifndef ROOM_WIFI_IDENTITY_PROVIDER_H
#define ROOM_WIFI_IDENTITY_PROVIDER_H
#include "room_wifi_apsta_link.h"
/* One explicit local initialization after protected store initialization,
 * before binding the Wi-Fi adapter, on the same task. No startup caller.
 * The result is valid for the lifetime of that task; never move ownership. */
bool room_wifi_identity_provider_init(room_wifi_apsta_provider *);
#endif
