#ifndef MOCK_WIFI_DEFAULT_H
#define MOCK_WIFI_DEFAULT_H
#include "esp_netif.h"
int esp_netif_attach_wifi_ap(esp_netif_t *);
int esp_netif_attach_wifi_station(esp_netif_t *);
int esp_wifi_set_default_wifi_ap_handlers(void);
int esp_wifi_clear_default_wifi_driver_and_handlers(void *);
int esp_wifi_set_default_wifi_sta_handlers(void);
#endif
