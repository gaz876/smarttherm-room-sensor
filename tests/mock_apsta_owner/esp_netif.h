#ifndef MOCK_NETIF_H
#define MOCK_NETIF_H
#include <stdbool.h>
#include <stdint.h>
typedef struct { unsigned type; } esp_netif_config_t;
typedef struct esp_netif esp_netif_t;
typedef struct { struct { uint32_t addr; } ip; } esp_netif_ip_info_t;
#define ESP_NETIF_DEFAULT_WIFI_AP() ((esp_netif_config_t){1})
#define ESP_NETIF_DEFAULT_WIFI_STA() ((esp_netif_config_t){2})
esp_netif_t *esp_netif_new(const esp_netif_config_t *);
void esp_netif_destroy(esp_netif_t *);
bool esp_netif_is_netif_up(esp_netif_t *);
int esp_netif_get_ip_info(esp_netif_t *,esp_netif_ip_info_t *);
typedef enum { ESP_NETIF_DHCP_INIT, ESP_NETIF_DHCP_STARTED, ESP_NETIF_DHCP_STOPPED } esp_netif_dhcp_status_t;
int esp_netif_dhcpc_get_status(esp_netif_t *,esp_netif_dhcp_status_t *);
int esp_netif_init(void);
#endif
