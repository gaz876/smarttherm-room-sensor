#ifndef MOCK_ESP_WIFI_H
#define MOCK_ESP_WIFI_H
#include "esp_http_server.h"
#include <stdint.h>
typedef struct { uint8_t mac[6]; } wifi_sta_info_t;
typedef struct {wifi_sta_info_t sta[2]; int num;} wifi_sta_list_t;
esp_err_t esp_wifi_ap_get_sta_list(wifi_sta_list_t *);
#endif
