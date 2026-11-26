#include "room_apsta_prepare.h"
#include <assert.h>
#include <stdio.h>
static unsigned calls, fail_at;
static int step(void) { return ++calls==fail_at?ESP_FAIL:ESP_OK; }
int esp_wifi_set_storage(wifi_storage_t v) { assert(!calls&&v==WIFI_STORAGE_RAM); return step(); }
int esp_wifi_set_mode(wifi_mode_t v) { assert(calls==1&&v==WIFI_MODE_APSTA); return step(); }
int esp_wifi_set_config(wifi_interface_t i,wifi_config_t *c) {
    if (i==WIFI_IF_AP) {
        assert(calls==2&&c->ap.ssid_len==1&&c->ap.ssid[0]=='x');
        assert(c->ap.authmode==WIFI_AUTH_WPA2_PSK&&c->ap.pairwise_cipher==WIFI_CIPHER_TYPE_CCMP);
        assert(c->ap.max_connection==1&&c->ap.channel==1&&c->ap.pmf_cfg.capable);
        assert(c->ap.password[16]==0);
    } else {
        assert(calls>=3&&i==WIFI_IF_STA&&c->sta.ssid[0]=='y');
        assert(c->sta.threshold.authmode==WIFI_AUTH_WPA2_PSK&&c->sta.pmf_cfg.capable);
        assert(c->sta.password[8]==0);
    }
    return step();
}
int main(void) {
    const uint8_t ssid[]={'x'}, key[16]={'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p'};
    room_enroll_profile p={.ssid_len=1,.pass_len=8,.ssid={'y'},.pass={'a','b','c','d','e','f','g','h'}};
    room_apsta_prepare_state s={0};
    assert(!room_apsta_prepare(NULL,ssid,1,key,16));
    assert(!room_apsta_prepare(&s,ssid,1,key,15));
    assert(!room_apsta_prepare(&s,NULL,1,key,16));
    assert(!room_apsta_configure_trial(&s,&p)&&calls==0);
    for (unsigned i=1;i<=3;i++) {
        s=(room_apsta_prepare_state){0};calls=0;fail_at=i;
        assert(!room_apsta_prepare(&s,ssid,1,key,16)&&s.failed&&calls==i);
        assert(!room_apsta_prepare(&s,ssid,1,key,16)&&calls==i);
        assert(!room_apsta_configure_trial(&s,&p));
    }
    s=(room_apsta_prepare_state){0};calls=0;fail_at=0;
    assert(room_apsta_prepare(&s,ssid,1,key,16)&&calls==3);
    assert(!room_apsta_prepare(&s,ssid,1,key,16)&&calls==3);
    assert(!room_apsta_configure_trial(&s,NULL)&&calls==3);
    assert(room_apsta_configure_trial(&s,&p)&&calls==4);
    fail_at=5; assert(!room_apsta_configure_trial(&s,&p)&&s.failed&&calls==5);
    assert(!room_apsta_configure_trial(&s,&p)&&calls==5);
    puts("APSTA prepare mock PASS"); return 0;
}
