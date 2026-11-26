#ifndef ROOM_NETWORK_H
#define ROOM_NETWORK_H
#include <stdbool.h>
/* Called once, after default measurement NVS initialization. -1 disables. */
bool room_network_start(int key_id);
#endif
