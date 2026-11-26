#ifndef MOCK_WIFI_SDK_H
#define MOCK_WIFI_SDK_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_EVENT_DEFINE_BASE(x) const char *x=#x
#define GPIO_MODE_INPUT 1
#define GPIO_PULLUP_ENABLE 1
#define GPIO_PULLDOWN_DISABLE 0
#define GPIO_INTR_DISABLE 0
#define GPIO_NUM_9 9
typedef struct { uint64_t pin_bit_mask; int mode,pull_up_en,pull_down_en,intr_type; } gpio_config_t;
typedef const char *esp_event_base_t;
typedef void (*esp_event_handler_t)(void *,esp_event_base_t,int32_t,void *);
typedef struct { bool up; uint32_t ip; } esp_netif_t;
typedef struct { int placeholder; } esp_netif_config_t;
typedef struct { struct { uint32_t addr; } ip; } esp_netif_ip_info_t;
typedef struct { int nvs_enable; } wifi_init_config_t;
typedef struct { struct { uint8_t ssid[32],password[64]; struct { int authmode; } threshold; struct { bool capable; } pmf_cfg; } sta; } wifi_config_t;
typedef struct { int placeholder; } wifi_ap_record_t;
#define WIFI_INIT_CONFIG_DEFAULT() ((wifi_init_config_t){1})
#define ESP_NETIF_DEFAULT_WIFI_STA() ((esp_netif_config_t){0})
#define WIFI_STORAGE_RAM 1
#define WIFI_MODE_STA 1
#define WIFI_IF_STA 0
#define WIFI_AUTH_WPA2_PSK 3
esp_err_t gpio_config(const gpio_config_t *);
int gpio_get_level(int);
int64_t esp_timer_get_time(void);
esp_err_t esp_netif_init(void);
esp_err_t esp_event_loop_create_default(void);
esp_err_t esp_event_handler_register(esp_event_base_t,int32_t,esp_event_handler_t,void *);
esp_err_t esp_event_post(esp_event_base_t,int32_t,const void *,size_t,unsigned);
esp_err_t esp_wifi_init(const wifi_init_config_t *);
esp_err_t esp_wifi_set_storage(int);
esp_err_t esp_wifi_set_mode(int);
esp_netif_t *esp_netif_new(const esp_netif_config_t *);
esp_err_t esp_netif_attach_wifi_station(esp_netif_t *);
esp_err_t esp_wifi_set_default_wifi_sta_handlers(void);
esp_err_t esp_wifi_set_config(int,wifi_config_t *);
esp_err_t esp_wifi_start(void);
esp_err_t esp_wifi_connect(void);
esp_err_t esp_wifi_stop(void);
esp_err_t esp_wifi_clear_default_wifi_driver_and_handlers(void *);
void esp_netif_destroy(esp_netif_t *);
esp_err_t esp_wifi_deinit(void);
esp_err_t esp_wifi_sta_get_ap_info(wifi_ap_record_t *);
bool esp_netif_is_netif_up(esp_netif_t *);
esp_err_t esp_netif_get_ip_info(esp_netif_t *,esp_netif_ip_info_t *);
#endif
