#ifndef MOCK_NETIF_H
#define MOCK_NETIF_H
#include "esp_http_server.h"
#include <stdint.h>
typedef struct esp_netif esp_netif_t;
esp_err_t esp_netif_get_netif_impl_name(esp_netif_t *,char *);
typedef struct {uint32_t addr;} esp_ip4_addr_t;
typedef struct {esp_ip4_addr_t ip,netmask,gw;} esp_netif_ip_info_t;
typedef struct {uint8_t mac[6];esp_ip4_addr_t ip;} esp_netif_pair_mac_ip_t;
typedef enum {ESP_NETIF_DHCP_INIT,ESP_NETIF_DHCP_STARTED} esp_netif_dhcp_status_t;
bool esp_netif_is_netif_up(esp_netif_t *);
esp_err_t esp_netif_get_ip_info(esp_netif_t *,esp_netif_ip_info_t *);
esp_err_t esp_netif_dhcps_get_status(esp_netif_t *,esp_netif_dhcp_status_t *);
esp_err_t esp_netif_dhcps_get_clients_by_mac(esp_netif_t *,int,esp_netif_pair_mac_ip_t *);
#endif
