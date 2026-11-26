#ifndef MOCK_EVENT_H
#define MOCK_EVENT_H
#include "esp_netif.h"
#include <stdint.h>
typedef const char *esp_event_base_t;
typedef void *esp_event_handler_instance_t;
typedef void (*esp_event_handler_t)(void *,esp_event_base_t,int32_t,void *);
extern const char ip_base[],wifi_base[];
#define IP_EVENT ip_base
#define WIFI_EVENT wifi_base
#define IP_EVENT_STA_GOT_IP 0
#define IP_EVENT_STA_LOST_IP 1
#define WIFI_EVENT_STA_DISCONNECTED 5
#define ESP_EVENT_ANY_ID (-1)
typedef struct { esp_netif_t *esp_netif; esp_netif_ip_info_t ip_info; } ip_event_got_ip_t;
int esp_event_handler_instance_register(esp_event_base_t,int32_t,esp_event_handler_t,void *,esp_event_handler_instance_t *);
int esp_event_handler_instance_unregister(esp_event_base_t,int32_t,esp_event_handler_instance_t);
#define ESP_EVENT_DEFINE_BASE(x) static const char x[]=#x
int esp_event_post(esp_event_base_t,int32_t,const void *,size_t,unsigned);
#endif
