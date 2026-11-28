#ifndef MOCK_APSTA_TASK_H
#define MOCK_APSTA_TASK_H
#include "room_apsta_events.h"
#include "room_wifi_idf.h"
enum {ROOM_SEC2_SALT_MAX=32,ROOM_SEC2_VERIFIER_SIZE=384};
typedef struct {const char *salt;uint16_t salt_len;const char *verifier;uint16_t verifier_len;} protocomm_security2_params_t;
typedef enum {PHONE_PROFILE_MISSING,PHONE_PROFILE_VALID} phone_profile_state;
typedef struct {room_apsta_session session;room_apsta_events events;bool ready,failed,pending;} room_apsta_task;
bool room_apsta_task_bind(room_apsta_task *);
bool room_apsta_task_open(room_apsta_task *,phone_profile_state,bool,bool,const room_enroll_store *,const uint8_t *,size_t,const uint8_t *,size_t,const protocomm_security2_params_t *,uint32_t);
bool room_apsta_task_poll(room_apsta_task *);
bool room_apsta_task_close_until(room_apsta_task *,bool,uint64_t);
#endif
