#ifndef MOCK_PHONE_CHANNEL_H
#define MOCK_PHONE_CHANNEL_H
#include "room_apsta_task.h"
typedef struct {void *ctx;bool (*open)(void *,const esp_netif_t *,uint64_t);int (*receive)(void *,bool *,uint8_t *,size_t,size_t *);int (*send)(void *,const uint8_t *,size_t,size_t *);bool (*close)(void *);} room_phone_io;
typedef bool (*room_phone_timed_close)(void *,uint64_t,uint64_t);
typedef bool (*room_phone_clock)(void *,uint64_t *);
typedef struct {uint64_t window;bool attempted;} room_phone_channel;
bool room_phone_channel_open(room_phone_channel *,room_apsta_task *,room_phone_io);
bool room_phone_channel_poll(room_phone_channel *);
bool room_phone_channel_close_until(room_phone_channel *,uint64_t,room_phone_clock,void *,room_phone_timed_close);
#endif
