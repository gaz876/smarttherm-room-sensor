#ifndef ROOM_WIFI_APSTA_LINK_H
#define ROOM_WIFI_APSTA_LINK_H
#include "room_wifi_idf.h"
typedef struct esp_netif esp_netif_t;
/* Opaque here: the adapter never inspects the setup window. */
typedef struct mock_room_wifi_apsta_window room_wifi_apsta_window;
typedef struct { void *ctx; bool (*ready)(void *); bool (*prepare)(void *,room_wifi_apsta_window *); } room_wifi_apsta_provider;
enum { ROOM_PHONE_PACKET_MAX=4096 };
typedef enum { ROOM_PHONE_IO_WAIT, ROOM_PHONE_IO_OK, ROOM_PHONE_IO_ERROR } room_phone_io_result;
typedef struct { void *ctx; bool (*open)(void *,const esp_netif_t *,uint64_t); room_phone_io_result (*receive)(void *,bool *,uint8_t *,size_t,size_t *); room_phone_io_result (*send)(void *,const uint8_t *,size_t,size_t *); bool (*close)(void *); } room_phone_io;
typedef bool (*room_phone_timed_close)(void *,uint64_t,uint64_t);
bool room_wifi_apsta_link_bind(room_wifi_apsta_provider);
bool room_wifi_apsta_link_init_missing(room_wifi_apsta_provider);
bool room_wifi_apsta_link_attach_io_until(room_phone_io,room_phone_timed_close);
bool room_wifi_apsta_link_peer_epoch(void *,const esp_netif_t *,const uint8_t [6],uint64_t *);
#endif
