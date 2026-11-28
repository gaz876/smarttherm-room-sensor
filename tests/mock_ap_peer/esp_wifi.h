#ifndef MOCK_WIFI_H
#define MOCK_WIFI_H
#include "esp_event.h"
#define WIFI_EVENT_STA_DISCONNECTED 3
#define WIFI_EVENT_AP_START 4
#define WIFI_EVENT_AP_STOP 5
#define WIFI_EVENT_AP_STACONNECTED 6
#define WIFI_EVENT_AP_STADISCONNECTED 7
typedef struct {uint8_t mac[6];uint8_t aid;bool is_mesh_child;} wifi_event_ap_staconnected_t;
typedef struct {uint8_t mac[6];uint8_t aid;bool is_mesh_child;} wifi_event_ap_stadisconnected_t;
typedef struct {uint8_t mac[6];} wifi_sta_info_t;
typedef struct {wifi_sta_info_t sta[2];int num;} wifi_sta_list_t;
int esp_wifi_ap_get_sta_list(wifi_sta_list_t *);
#endif
