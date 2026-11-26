#include "room_apsta_owner.h"
#include <assert.h>
#include <stdio.h>
struct esp_netif { unsigned type; bool allocated,attached,up; uint32_t ip; };
static struct esp_netif ap,sta;
static bool driver,started,linked,pre,post,stay_up;
static unsigned calls,fail,stops,connects,destroys,deinits;
static int op(void) { return ++calls==fail?ESP_FAIL:ESP_OK; }
static void reset(void) { ap=(struct esp_netif){.type=1};sta=(struct esp_netif){.type=2};driver=started=linked=pre=post=stay_up=false;calls=fail=stops=connects=destroys=deinits=0; }
int esp_wifi_init(const wifi_init_config_t *c) { assert(!driver&&!c->nvs_enable);int r=op();if(!r)driver=true;return r; }
esp_netif_t *esp_netif_new(const esp_netif_config_t *c) { assert(driver);struct esp_netif *p=c->type==1?&ap:&sta;assert(!p->allocated);if(op())return NULL;p->allocated=true;return p; }
int esp_netif_attach_wifi_ap(esp_netif_t *p) { assert(p==&ap&&p->allocated);int r=op();if(!r)p->attached=true;return r; }
int esp_netif_attach_wifi_station(esp_netif_t *p) { assert(p==&sta&&p->allocated);int r=op();if(!r)p->attached=true;return r; }
int esp_wifi_set_default_wifi_ap_handlers(void) { assert(ap.attached&&sta.attached&&!started);return op(); }
int esp_wifi_set_storage(wifi_storage_t v) { assert(driver&&v==WIFI_STORAGE_RAM);return op(); }
int esp_wifi_set_mode(wifi_mode_t v) { assert(driver&&v==WIFI_MODE_APSTA);return op(); }
int esp_wifi_set_config(wifi_interface_t i,wifi_config_t *c) { assert(driver);if(i==WIFI_IF_AP)assert(!started&&c->ap.authmode==WIFI_AUTH_WPA2_PSK&&c->ap.max_connection==1);else assert(started&&c->sta.threshold.authmode==WIFI_AUTH_WPA2_PSK);return op(); }
int esp_wifi_start(void) { assert(driver&&ap.attached&&sta.attached);int r=op();started=true;ap.up=true;return r; }
int esp_wifi_connect(void) { assert(started&&sta.attached);connects++;return op(); }
int esp_wifi_sta_get_ap_info(wifi_ap_record_t *p) { (void)p;return linked?ESP_OK:ESP_FAIL; }
bool esp_netif_is_netif_up(esp_netif_t *p) { assert(p->allocated);return p->up; }
int esp_netif_get_ip_info(esp_netif_t *p,esp_netif_ip_info_t *i) { assert(p->allocated);i->ip.addr=p->ip;return ESP_OK; }
int esp_wifi_stop(void) { assert(driver&&started);stops++;int r=op();if(!r){started=false;if(!stay_up)ap.up=sta.up=false;}return r; }
int esp_wifi_clear_default_wifi_driver_and_handlers(void *v) { struct esp_netif *p=v;assert(pre&&!started&&p->attached&&!p->up);int r=op();if(!r)p->attached=false;return r; }
void esp_netif_destroy(esp_netif_t *p) { assert(!p->attached&&!p->up&&p->allocated&&pre);p->allocated=false;destroys++; }
int esp_wifi_deinit(void) { assert(driver&&!started&&!ap.allocated&&!sta.allocated);deinits++;int r=op();if(!r)driver=false;return r; }
static bool barrier(void *v,uint64_t w,bool final) { (void)v;assert(w);return final?post:pre; }
static const uint8_t ssid[]={'x'},key[16]={'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p'};
static const room_enroll_profile profile={.ssid_len=1,.pass_len=8,.ssid={'y'},.pass={'a','b','c','d','e','f','g','h'}};
#define START(w) room_apsta_start(&o,w,true,ssid,1,key,16)
#define STOP(w,d) room_apsta_stop(&o,w,d,barrier,NULL)
int main(void) {
    room_apsta_owner o={0};reset();
    assert(!room_apsta_start(&o,1,false,ssid,1,key,16)&&!calls);
    assert(START(1)&&calls==10&&room_apsta_ready(&o,1));
    assert(!START(2)&&!room_apsta_trial(&o,2,1,&profile));
    assert(room_apsta_trial(&o,1,1,&profile)&&connects==1);
    assert(!room_apsta_trial(&o,1,2,&profile)&&connects==1);
    assert(!room_apsta_link(&o,1,1));linked=true;sta.up=true;sta.ip=1;
    assert(room_apsta_link(&o,1,1)&&!room_apsta_link(&o,1,2));
    assert(!STOP(2,true)&&!stops);assert(!STOP(1,false)&&!stops&&!room_apsta_ready(&o,1));
    stay_up=true;assert(!STOP(1,true)&&stops==1);pre=true;assert(!STOP(1,true)&&!destroys);
    ap.up=sta.up=false;assert(!STOP(1,true)&&destroys==2&&deinits==1&&o.phase==APSTA_FINAL_DRAIN);
    assert(!START(2));post=true;assert(STOP(1,true)&&o.phase==APSTA_IDLE&&!driver);
    assert(!START(1));reset();assert(START(2));sta.ip=1;
    assert(!room_apsta_trial(&o,2,2,&profile)&&!connects);pre=post=true;assert(STOP(2,true));
    for(unsigned i=1;i<=10;i++) {
        reset();o=(room_apsta_owner){0};fail=i;assert(!START(1));assert(!START(2));pre=post=true;
        bool clean=STOP(1,true);bool ambiguous=i==1||i==3||i==5||i==6;
        assert(clean==!ambiguous);assert(o.phase==(ambiguous?APSTA_LOCKED:APSTA_IDLE));
        if(i==10)assert(stops==1);
    }
    for(unsigned i=11;i<=14;i++) {
        reset();o=(room_apsta_owner){0};assert(START(1));pre=post=true;fail=i;
        assert(!STOP(1,true)&&o.phase==APSTA_LOCKED);unsigned count=calls;
        assert(!STOP(1,true)&&!START(2)&&calls==count);
    }
    reset();o=(room_apsta_owner){0};assert(START(1));fail=12;
    assert(!room_apsta_trial(&o,1,1,&profile)&&connects==1&&o.phase==APSTA_STOP);
    pre=post=true;assert(STOP(1,true));
    puts("APSTA owner mock PASS");return 0;
}
