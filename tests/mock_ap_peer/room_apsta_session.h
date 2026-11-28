#ifndef MOCK_APSTA_SESSION_H
#define MOCK_APSTA_SESSION_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
typedef struct esp_netif {unsigned marker;} esp_netif_t;
typedef struct {uint64_t saved_generation,deadline;struct {bool storage_uncertain;} enrollment;} room_phone_setup;
enum {APSTA_IDLE,APSTA_RUNNING,APSTA_LOCKED};
typedef struct {unsigned lost,ip;bool locked;struct {room_phone_setup phone;uint64_t window,last_window;} bridge;struct {const esp_netif_t *ap;int phase;} radio;struct {bool bridge,pending,dispatching;} transport;} room_apsta_session;
void room_apsta_session_lost(room_apsta_session *);
void room_apsta_session_ip(room_apsta_session *,const esp_netif_t *,uint32_t);
#endif
