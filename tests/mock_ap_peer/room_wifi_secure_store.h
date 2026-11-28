#ifndef MOCK_SECURE_STORE_H
#define MOCK_SECURE_STORE_H
#include "room_wifi_idf.h"
typedef enum {ROOM_SEC_VALID,ROOM_SEC_MISSING} room_secure_status;
room_secure_status room_secure_load(room_enroll_profile *);
room_enroll_store room_secure_store(void);
#endif
