#ifndef ROOM_APSTA_PREPARE_H
#define ROOM_APSTA_PREPARE_H
#include "room_wifi_enrollment.h"
#include "esp_wifi.h"

/* Unlinked experimental SDK configuration helper. The exclusive owner must
 * initialize a fresh RAM-only driver (nvs_enable=0), attach both netifs, and
 * keep it stopped before prepare. No start/connect, listener or persistence.
 * Identity/key readiness and the physical setup window are checked by owner.
 * Partial SDK failure is quarantined; no retry or reset of this instance. */
typedef struct { bool prepared, failed; } room_apsta_prepare_state;
static inline bool room_apsta_prepare(room_apsta_prepare_state *s,
    const uint8_t *ssid,size_t n,const uint8_t *key,size_t m) {
    if (!s || s->prepared || s->failed || m<16 || !enroll_profile_valid(ssid,n,key,m)) return false;
    wifi_config_t ap={0};
    memcpy(ap.ap.ssid,ssid,n); memcpy(ap.ap.password,key,m);
    ap.ap.ssid_len=(uint8_t)n; ap.ap.channel=1;
    ap.ap.authmode=WIFI_AUTH_WPA2_PSK; ap.ap.pairwise_cipher=WIFI_CIPHER_TYPE_CCMP;
    ap.ap.max_connection=1; ap.ap.beacon_interval=100; ap.ap.pmf_cfg.capable=true;
    bool ok=esp_wifi_set_storage(WIFI_STORAGE_RAM)==ESP_OK &&
        esp_wifi_set_mode(WIFI_MODE_APSTA)==ESP_OK &&
        esp_wifi_set_config(WIFI_IF_AP,&ap)==ESP_OK;
    enroll_wipe(&ap,sizeof(ap));
    s->prepared=ok; s->failed=!ok; return ok;
}
/* Owner must prove the previous station trial has stopped/drained and pass
 * only the current authenticated candidate. Does not initiate association. */
static inline bool room_apsta_configure_trial(room_apsta_prepare_state *s,
    const room_enroll_profile *p) {
    if (!s || !s->prepared || s->failed || !p ||
        !enroll_profile_valid(p->ssid,p->ssid_len,p->pass,p->pass_len)) return false;
    wifi_config_t sta={0};
    memcpy(sta.sta.ssid,p->ssid,p->ssid_len); memcpy(sta.sta.password,p->pass,p->pass_len);
    sta.sta.threshold.authmode=WIFI_AUTH_WPA2_PSK; sta.sta.pmf_cfg.capable=true;
    bool ok=esp_wifi_set_config(WIFI_IF_STA,&sta)==ESP_OK;
    enroll_wipe(&sta,sizeof(sta));
    if (!ok) s->failed=true;
    return ok;
}
#endif
