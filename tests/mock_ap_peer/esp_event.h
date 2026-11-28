#ifndef MOCK_EVENT_H
#define MOCK_EVENT_H
#include "room_apsta_session.h"
typedef const void *esp_event_base_t;
typedef void *esp_event_handler_instance_t;
typedef void (*esp_event_handler_t)(void *,esp_event_base_t,int32_t,void *);
extern int ip_base,wifi_base;
#define IP_EVENT ((esp_event_base_t)&ip_base)
#define WIFI_EVENT ((esp_event_base_t)&wifi_base)
#define ESP_EVENT_ANY_ID (-1)
#define ESP_OK 0
#define IP_EVENT_STA_GOT_IP 1
#define IP_EVENT_STA_LOST_IP 2
typedef struct {esp_netif_t *esp_netif;struct {struct {uint32_t addr;} ip;} ip_info;} ip_event_got_ip_t;
int esp_event_handler_instance_register(esp_event_base_t,int32_t,esp_event_handler_t,void *,esp_event_handler_instance_t *);
int esp_event_handler_instance_unregister(esp_event_base_t,int32_t,esp_event_handler_instance_t);
#endif
