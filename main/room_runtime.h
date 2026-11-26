#ifndef ROOM_RUNTIME_H
#define ROOM_RUNTIME_H
#include "room_measure.h"
#include "room_settings.h"
/* Task context only. Single measurement owner; readers receive synchronized copies. */
room_status room_runtime_snapshot(sht40_sample *out, room_settings_result *settings_status);
/* Internal command API: only a future authenticated caller may expose it. One outstanding
 * request; the latest result remains available until a new request is accepted. */
typedef enum { ROOM_COMMAND_UNKNOWN, ROOM_COMMAND_PENDING, ROOM_COMMAND_DONE } room_command_state;
bool room_runtime_request_settings(room_settings desired, bool reset, uint32_t *ticket);
room_command_state room_runtime_settings_result(uint32_t ticket, room_settings_result *result);
bool room_runtime_settings(room_settings *active, room_settings_result *status);
#endif
