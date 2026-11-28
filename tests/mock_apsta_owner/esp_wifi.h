#ifndef MOCK_ESP_WIFI_H
#define MOCK_ESP_WIFI_H
#include <stdbool.h>
#include <stdint.h>
#define ESP_OK 0
#define ESP_FAIL (-1)
typedef enum { WIFI_STORAGE_FLASH, WIFI_STORAGE_RAM } wifi_storage_t;
typedef enum { WIFI_MODE_NULL, WIFI_MODE_STA, WIFI_MODE_AP, WIFI_MODE_APSTA } wifi_mode_t;
typedef enum { WIFI_IF_STA, WIFI_IF_AP } wifi_interface_t;
typedef enum { WIFI_AUTH_OPEN, WIFI_AUTH_WPA2_PSK } wifi_auth_mode_t;
typedef enum { WIFI_CIPHER_TYPE_NONE, WIFI_CIPHER_TYPE_CCMP } wifi_cipher_type_t;
typedef struct { bool capable, required; } wifi_pmf_config_t;
typedef union {
    struct { uint8_t ssid[32],password[64],ssid_len,channel,max_connection; uint16_t beacon_interval;
        wifi_auth_mode_t authmode; wifi_cipher_type_t pairwise_cipher; wifi_pmf_config_t pmf_cfg; } ap;
    struct { uint8_t ssid[32],password[64]; struct { wifi_auth_mode_t authmode; } threshold;
        wifi_pmf_config_t pmf_cfg; } sta;
} wifi_config_t;
int esp_wifi_set_storage(wifi_storage_t);
int esp_wifi_set_mode(wifi_mode_t);
int esp_wifi_set_config(wifi_interface_t,wifi_config_t *);
typedef struct { int nvs_enable; } wifi_init_config_t;
#define WIFI_INIT_CONFIG_DEFAULT() ((wifi_init_config_t){1})
typedef struct { int dummy; } wifi_ap_record_t;
int esp_wifi_init(const wifi_init_config_t *);
int esp_wifi_start(void);
int esp_wifi_stop(void);
int esp_wifi_deinit(void);
int esp_wifi_connect(void);
int esp_wifi_sta_get_ap_info(wifi_ap_record_t *);
#define WIFI_EVENT_AP_START 10
#define WIFI_EVENT_AP_STOP 11
#define WIFI_EVENT_AP_STACONNECTED 12
#define WIFI_EVENT_AP_STADISCONNECTED 13
typedef struct { uint8_t mac[6],aid; bool is_mesh_child; } wifi_event_ap_staconnected_t;
#endif
