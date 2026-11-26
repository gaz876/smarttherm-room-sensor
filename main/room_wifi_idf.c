#include "room_wifi_idf.h"
#include "room_wifi_owner.h"
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include <stdatomic.h>
#include <string.h>

ESP_EVENT_DEFINE_BASE(ROOM_WIFI_BARRIER);
typedef enum { CLEAN_NONE,CLEAN_STOPPED,CLEAN_DETACHED,CLEAN_DEINIT } clean_state;
static struct {
    room_wifi_owner owner;
    room_boot_button button;
    wifi_config_t config;
    esp_netif_t *netif;
    uint64_t session;
    uint32_t posted,serial;
    atomic_uint ack;
    clean_state clean;
    bool once,ready,fatal,driver,attempted;
} w;
static uint64_t now_ms(void) { return (uint64_t)(esp_timer_get_time()/1000); }
static void barrier_handler(void *arg,esp_event_base_t base,int32_t id,void *data) {
    (void)arg;(void)base;(void)id;
    uint32_t token;
    memcpy(&token,data,sizeof(token));
    atomic_store_explicit(&w.ack,token,memory_order_release);
}
/* Called in owner task, never the default event loop. Do not block on the queue.
 * Acknowledgement drains already-posted events; it is not a hardware proof that
 * no vendor task can produce a delayed event after stop/deinit. */
static bool barrier(void) {
    if (!w.posted) {
        if (w.serial==UINT32_MAX) { w.fatal=true; return false; }
        uint32_t token=++w.serial;
        if (esp_event_post(ROOM_WIFI_BARRIER,0,&token,sizeof(token),0)!=ESP_OK) return false;
        w.posted=token;
    }
    if (atomic_load_explicit(&w.ack,memory_order_acquire)!=w.posted) return false;
    w.posted=0;
    return true;
}
static bool stop(void *ctx) {
    (void)ctx; w.session=0;
    if (w.fatal) return false;
    if (w.clean==CLEAN_NONE) {
        if (w.attempted && esp_wifi_stop()!=ESP_OK) return false;
        w.attempted=false; w.clean=CLEAN_STOPPED;
    }
    if (w.clean==CLEAN_STOPPED) {
        if (!barrier()) return false;
        if (w.netif) {
            if (esp_netif_is_netif_up(w.netif)) return false;
            /* The SDK clear function can detach even on error; never repeat an
             * ambiguous detach, never reuse that interface. */
            if (esp_wifi_clear_default_wifi_driver_and_handlers(w.netif)!=ESP_OK) { w.fatal=true; return false; }
            esp_netif_destroy(w.netif); w.netif=NULL;
        }
        w.clean=CLEAN_DETACHED;
    }
    if (w.clean==CLEAN_DETACHED) {
        if (w.driver && esp_wifi_deinit()!=ESP_OK) return false;
        w.driver=false; w.clean=CLEAN_DEINIT;
    }
    if (!barrier()) return false;
    w.clean=CLEAN_NONE;
    return true;
}
static bool station(void *ctx,uint64_t token) {
    (void)ctx;
    if (w.fatal || w.driver || w.netif || w.clean!=CLEAN_NONE) return false;
    wifi_init_config_t init=WIFI_INIT_CONFIG_DEFAULT();
    init.nvs_enable=0;
    if (esp_wifi_init(&init)!=ESP_OK) { w.fatal=true; return false; }
    w.driver=true;
    if (esp_wifi_set_storage(WIFI_STORAGE_RAM)!=ESP_OK || esp_wifi_set_mode(WIFI_MODE_STA)!=ESP_OK) return false;
    esp_netif_config_t cfg=ESP_NETIF_DEFAULT_WIFI_STA();
    w.netif=esp_netif_new(&cfg);
    if (!w.netif) return false;
    /* Attach failure can leave SDK-global state partially assigned. Fail closed
     * instead of guessing at a safe repeat/detach; measurement task is separate. */
    if (esp_netif_attach_wifi_station(w.netif)!=ESP_OK) { w.fatal=true; return false; }
    if (esp_wifi_set_default_wifi_sta_handlers()!=ESP_OK) return false;
    if (esp_wifi_set_config(WIFI_IF_STA,&w.config)!=ESP_OK) return false;
    w.attempted=true;
    if (esp_wifi_start()!=ESP_OK || esp_wifi_connect()!=ESP_OK) return false;
    w.session=token;
    return true;
}
static bool setup(void *ctx,uint64_t token) { (void)ctx;(void)token;return false; }
static bool profile_valid(const uint8_t *ssid,size_t n,const uint8_t *pass,size_t m) {
    if (!ssid || !pass || !n || n>32 || m<8 || m>63) return false;
    for (size_t i=0;i<n;i++) if (!ssid[i]) return false;
    for (size_t i=0;i<m;i++) if (pass[i]<32 || pass[i]>126) return false;
    return true;
}
bool room_wifi_idf_init(const uint8_t *ssid,size_t n,const uint8_t *pass,size_t m) {
    if (w.once || !profile_valid(ssid,n,pass,m)) return false;
    w.once=true;
    /* Exclusive networking owner. Existing default loop is not accepted. */
    gpio_config_t gpio={.pin_bit_mask=UINT64_C(1)<<9,.mode=GPIO_MODE_INPUT,
        .pull_up_en=GPIO_PULLUP_ENABLE,.pull_down_en=GPIO_PULLDOWN_DISABLE,.intr_type=GPIO_INTR_DISABLE};
    if (gpio_config(&gpio)!=ESP_OK || esp_netif_init()!=ESP_OK || esp_event_loop_create_default()!=ESP_OK ||
        esp_event_handler_register(ROOM_WIFI_BARRIER,0,barrier_handler,NULL)!=ESP_OK) return false;
    memcpy(w.config.sta.ssid,ssid,n); memcpy(w.config.sta.password,pass,m);
    w.config.sta.threshold.authmode=WIFI_AUTH_WPA2_PSK;
    w.config.sta.pmf_cfg.capable=true;
    wifi_owner_init(&w.owner,(room_wifi_transport){NULL,stop,station,setup},ROOM_WIFI_PROFILE_VALID,now_ms(),false);
    w.ready=true;
    return true;
}
static room_wifi_event observe(void) {
    if (!w.session || !w.netif) return (room_wifi_event){0};
    wifi_ap_record_t ap;
    esp_netif_ip_info_t ip;
    /* Fresh netif on each attempt, DHCP default. Raw SDK IP/disconnect events
     * are never relabelled with the current token. Observe live link + live IP. */
    bool live=esp_wifi_sta_get_ap_info(&ap)==ESP_OK && esp_netif_is_netif_up(w.netif) &&
        esp_netif_get_ip_info(w.netif,&ip)==ESP_OK && ip.ip.addr!=0;
    if (live) return (room_wifi_event){WIFI_EVENT_IP,w.session};
    if (wifi_owner_online(&w.owner)) return (room_wifi_event){WIFI_EVENT_LOST,w.session};
    return (room_wifi_event){0};
}
bool room_wifi_idf_step(bool *manual_request) {
    if (manual_request) *manual_request=false;
    if (!w.ready) return false;
    room_wifi_event event=observe();
    uint64_t now=now_ms();
    bool manual=wifi_boot_sample(&w.button,gpio_get_level(GPIO_NUM_9)==0,now);
    if (manual_request) *manual_request=manual;
    /* No provisioning transport yet: BOOT only reports a local gesture. */
    wifi_owner_step(&w.owner,now,false,false,event);
    return wifi_owner_online(&w.owner);
}
