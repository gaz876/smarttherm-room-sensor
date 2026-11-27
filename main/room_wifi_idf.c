#include "room_wifi_idf.h"
#include "room_wifi_owner.h"
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "freertos/task.h"
#include <stdatomic.h>
#include <string.h>

ESP_EVENT_DEFINE_BASE(ROOM_WIFI_BARRIER);
typedef enum { CLEAN_NONE,CLEAN_STOPPED,CLEAN_DETACHED,CLEAN_DEINIT } clean_state;
static struct {
    room_wifi_owner owner;
    room_boot_button button;
    room_wifi_setup_io setup_io;
    TaskHandle_t task;
    uint64_t cleanup_deadline;
    bool bind_attempted, bound, apsta, stepped, automatic;
    wifi_config_t config;
    esp_netif_t *netif;
    uint64_t session;
    uint32_t posted,serial;
    atomic_uint ack;
    clean_state clean;
    bool once,ready,fatal,driver,attempted;
} w;
static uint64_t now_ms(void) { return (uint64_t)(esp_timer_get_time()/1000); }
bool room_wifi_idf_is_owner(void) {
    return w.ready && w.task && w.task==xTaskGetCurrentTaskHandle();
}
bool room_wifi_idf_bind_setup(room_wifi_setup_io io) {
    if (!room_wifi_idf_is_owner() || w.fatal || w.bind_attempted ||
        w.stepped || w.owner.generation || w.owner.dirty || w.driver || w.netif ||
        !io.bind || !io.ready || !io.start || !io.poll || !io.stop || !io.load) return false;
    w.bind_attempted=true;
    if (!io.bind(io.ctx)) return false;
    w.setup_io=io; w.bound=true;
    /* Do not retain the old STA credentials when a protected reload exists. */
    enroll_wipe(&w.config,sizeof(w.config));
    return true;
}
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
    uint64_t now=now_ms();
    if (!w.cleanup_deadline) w.cleanup_deadline=wifi_deadline(now,5000u);
    if (now>=w.cleanup_deadline) { w.fatal=true; return false; }
    if (w.apsta) {
        room_wifi_setup_result result=ROOM_WIFI_SETUP_NO_SAVE;
        if (!w.setup_io.stop(w.setup_io.ctx,&result)) return false;
        w.apsta=false; w.cleanup_deadline=0;
        /* Completion is reconciled only after radio and transport teardown.
         * A verified write survives loss of the final response or timeout.
         * No saved profile is inferred merely from closing the setup window. */
        if (result==ROOM_WIFI_SETUP_COMMITTED) {
            w.owner.pending_setup=false;
            w.owner.policy.profile=ROOM_WIFI_PROFILE_VALID;
            w.owner.policy.state=ROOM_WIFI_RETRY;
            w.owner.policy.retry_ms=1000u;
            w.owner.policy.deadline=now_ms();
        } else if (result!=ROOM_WIFI_SETUP_NO_SAVE) {
            w.owner.pending_setup=false;
            w.owner.policy.profile=ROOM_WIFI_PROFILE_ERROR;
            w.owner.policy.state=ROOM_WIFI_OFF;
            enroll_wipe(&w.config,sizeof(w.config));
            w.fatal=true;
        }
        return true;
    }
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
    w.clean=CLEAN_NONE; w.cleanup_deadline=0;
    if (w.bound) enroll_wipe(&w.config,sizeof(w.config));
    return true;
}
static void configure(const uint8_t *ssid,size_t n,const uint8_t *pass,size_t m) {
    enroll_wipe(&w.config,sizeof(w.config));
    memcpy(w.config.sta.ssid,ssid,n); memcpy(w.config.sta.password,pass,m);
    w.config.sta.threshold.authmode=WIFI_AUTH_WPA2_PSK;
    w.config.sta.pmf_cfg.capable=true;
}
static bool station(void *ctx,uint64_t token) {
    (void)ctx;
    if (w.fatal || w.apsta || w.driver || w.netif || w.posted ||
        w.clean!=CLEAN_NONE || w.cleanup_deadline) return false;
    if (w.bound) {
        room_enroll_profile p={0};
        bool ok=w.setup_io.load(w.setup_io.ctx,&p) &&
            enroll_profile_valid(p.ssid,p.ssid_len,p.pass,p.pass_len);
        if (ok) configure(p.ssid,p.ssid_len,p.pass,p.pass_len);
        enroll_wipe(&p,sizeof(p));
        if (!ok) { w.fatal=true; return false; }
    }
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
static bool setup(void *ctx,uint64_t token) {
    (void)ctx;(void)token;
    if (!w.bound || w.fatal || w.apsta || w.driver || w.netif || w.posted ||
        w.clean!=CLEAN_NONE || w.cleanup_deadline ||
        now_ms()>=w.owner.policy.deadline || !w.setup_io.ready(w.setup_io.ctx)) return false;
    /* Claim the mode before calling start, even if start later fails. */
    w.apsta=true;
    return w.setup_io.start(w.setup_io.ctx,w.owner.policy.deadline,w.owner.policy.profile,!w.automatic);
}
static bool profile_valid(const uint8_t *ssid,size_t n,const uint8_t *pass,size_t m) {
    if (!ssid || !pass || !n || n>32 || m<8 || m>63) return false;
    for (size_t i=0;i<n;i++) if (!ssid[i]) return false;
    for (size_t i=0;i<m;i++) if (pass[i]<32 || pass[i]>126) return false;
    return true;
}
static bool init(room_wifi_profile profile,const uint8_t *ssid,size_t n,const uint8_t *pass,size_t m) {
    if (w.once || (profile!=ROOM_WIFI_PROFILE_MISSING &&
        (profile!=ROOM_WIFI_PROFILE_VALID || !profile_valid(ssid,n,pass,m)))) return false;
    w.once=true; w.task=xTaskGetCurrentTaskHandle();
    if (!w.task) return false;
    /* Exclusive networking owner. Existing default loop is not accepted. */
    gpio_config_t gpio={.pin_bit_mask=UINT64_C(1)<<9,.mode=GPIO_MODE_INPUT,
        .pull_up_en=GPIO_PULLUP_ENABLE,.pull_down_en=GPIO_PULLDOWN_DISABLE,.intr_type=GPIO_INTR_DISABLE};
    if (gpio_config(&gpio)!=ESP_OK || esp_netif_init()!=ESP_OK || esp_event_loop_create_default()!=ESP_OK ||
        esp_event_handler_register(ROOM_WIFI_BARRIER,0,barrier_handler,NULL)!=ESP_OK) return false;
    if (profile==ROOM_WIFI_PROFILE_VALID) configure(ssid,n,pass,m);
    wifi_owner_init(&w.owner,(room_wifi_transport){NULL,stop,station,setup},profile,now_ms(),false);
    w.ready=true;
    return true;
}
bool room_wifi_idf_init(const uint8_t *ssid,size_t n,const uint8_t *pass,size_t m) {
    return init(ROOM_WIFI_PROFILE_VALID,ssid,n,pass,m);
}
bool room_wifi_idf_init_missing(void) {
    return init(ROOM_WIFI_PROFILE_MISSING,NULL,0,NULL,0);
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
    if (!room_wifi_idf_is_owner() || w.fatal) return false;
    room_wifi_event event=observe();
    uint64_t now=now_ms();
    bool manual=wifi_boot_sample(&w.button,gpio_get_level(GPIO_NUM_9)==0,now);
    if (manual_request) *manual_request=manual;
    bool ready=w.bound && w.setup_io.ready(w.setup_io.ctx);
    /* One boot opportunity, consumed even when identity/store is unavailable.
     * No deferred automatic AP when readiness later changes. */
    if (!w.stepped) {
        w.stepped=true;
        if (ready && w.owner.policy.profile==ROOM_WIFI_PROFILE_MISSING &&
            now!=UINT64_MAX) {
            w.automatic=true;
            wifi_owner_action(&w.owner,wifi_setup(&w.owner.policy,now,true),now);
            w.automatic=false;
            return false;
        }
    }
    if (w.apsta && w.owner.active &&
        (!ready || !w.setup_io.poll(w.setup_io.ctx)))
        event=(room_wifi_event){WIFI_EVENT_SETUP_ABORTED,w.owner.active};
    /* Without an explicit local binding, BOOT still only reports a gesture. */
    wifi_owner_step(&w.owner,now,manual && w.bound,ready,event);
    return wifi_owner_online(&w.owner);
}
