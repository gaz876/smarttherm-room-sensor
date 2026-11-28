#ifndef MOCK_ESP_NETIF_H
#define MOCK_ESP_NETIF_H
#include "room_apsta_session.h"
typedef struct {uint32_t addr;} mock_ip_addr;
typedef struct {mock_ip_addr ip;} esp_netif_ip_info_t;
typedef struct {uint8_t mac[6];mock_ip_addr ip;} esp_netif_pair_mac_ip_t;
typedef enum {ESP_NETIF_DHCP_INIT,ESP_NETIF_DHCP_STARTED} esp_netif_dhcp_status_t;
bool esp_netif_is_netif_up(esp_netif_t *);
int esp_netif_get_ip_info(esp_netif_t *,esp_netif_ip_info_t *);
int esp_netif_dhcps_get_status(esp_netif_t *,esp_netif_dhcp_status_t *);
int esp_netif_dhcps_get_clients_by_mac(esp_netif_t *,size_t,esp_netif_pair_mac_ip_t *);
#endif
