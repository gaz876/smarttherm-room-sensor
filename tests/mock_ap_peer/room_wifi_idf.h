#ifndef MOCK_WIFI_IDF_H
#define MOCK_WIFI_IDF_H
#include "room_apsta_session.h"
#include <string.h>
typedef struct {uint8_t ssid_len,pass_len;uint8_t ssid[32],pass[63];} room_enroll_profile;
typedef struct {void *ctx;bool (*ready)(void *);} room_enroll_store;
static inline bool enroll_ready(const room_enroll_store *s){return s&&s->ready&&s->ready(s->ctx);}
static inline void enroll_wipe(void *p,size_t n){memset(p,0,n);}
static inline bool enroll_profile_valid(const uint8_t *s,size_t n,const uint8_t *p,size_t m){return s&&p&&n>0&&n<=32&&m>=8&&m<=63;}
typedef enum {ROOM_WIFI_PROFILE_VALID,ROOM_WIFI_PROFILE_MISSING} room_wifi_profile;
typedef enum {ROOM_WIFI_SETUP_NO_SAVE,ROOM_WIFI_SETUP_COMMITTED,ROOM_WIFI_SETUP_UNCERTAIN} room_wifi_setup_result;
typedef struct {void *ctx;bool (*bind)(void *);bool (*ready)(void *);bool (*start)(void *,uint64_t,room_wifi_profile,bool);bool (*poll)(void *);bool (*stop)(void *,uint64_t,room_wifi_setup_result *);bool (*load)(void *,room_enroll_profile *);} room_wifi_setup_io;
bool room_wifi_idf_is_owner(void);
bool room_wifi_idf_init_missing(void);
bool room_wifi_idf_bind_setup(room_wifi_setup_io);
#endif
