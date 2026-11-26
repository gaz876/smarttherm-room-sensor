#ifndef ROOM_RUNTIME_H
#define ROOM_RUNTIME_H
#include "room_measure.h"
#include "room_settings.h"
/* Task context only. Single measurement owner; readers receive synchronized copies. */
room_status room_runtime_snapshot(sht40_sample *out, room_settings_result *settings_status);
#endif
