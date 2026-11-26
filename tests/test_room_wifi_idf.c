#include "mock_wifi_sdk.h"
#include "room_wifi_idf.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint64_t clock_ms;
static bool initialized,allocated,attached,handlers,started,linked,down,fail_stop,fail_attach,fail_start,fail_post;
static unsigned starts,stops,allocs,deinits;
static uint32_t queued;
static esp_event_handler_t cb;
static esp_event_base_t base;
static esp_netif_t ni;
esp_err_t gpio_config(const gpio_config_t *c) { assert(c->pin_bit_mask==(UINT64_C(1)<<9) && c->mode==GPIO_MODE_INPUT && c->pull_up_en);return 0; }
int gpio_get_level(int pin) { assert(pin==9);return !down; }
int64_t esp_timer_get_time(void) { return (int64_t)clock_ms*1000; }
esp_err_t esp_netif_init(void) { return 0; }
esp_err_t esp_event_loop_create_default(void) { return 0; }
esp_err_t esp_event_handler_register(esp_event_base_t b,int32_t id,esp_event_handler_t f,void *p) { assert(!id && !p);base=b;cb=f;return 0; }
esp_err_t esp_event_post(esp_event_base_t b,int32_t id,const void *p,size_t n,unsigned ticks) { assert(b==base && !id && n==4 && !queued && !ticks);if(fail_post){fail_post=false;return 1;}memcpy(&queued,p,n);return 0; }
static void deliver(void) { assert(queued);uint32_t token=queued;queued=0;cb(NULL,base,0,&token); }
esp_err_t esp_wifi_init(const wifi_init_config_t *c) { assert(!initialized && !c->nvs_enable);initialized=true;return 0; }
esp_err_t esp_wifi_set_storage(int n) { assert(initialized && n==WIFI_STORAGE_RAM);return 0; }
esp_err_t esp_wifi_set_mode(int n) { assert(initialized && n==WIFI_MODE_STA);return 0; }
esp_netif_t *esp_netif_new(const esp_netif_config_t *c) { assert(c && initialized && !allocated);allocated=true;++allocs;ni=(esp_netif_t){0};return &ni; }
esp_err_t esp_netif_attach_wifi_station(esp_netif_t *p) { assert(p==&ni && allocated && !attached);attached=true;return fail_attach?1:0; }
esp_err_t esp_wifi_set_default_wifi_sta_handlers(void) { assert(attached);handlers=true;return 0; }
esp_err_t esp_wifi_set_config(int i,wifi_config_t *c) { assert(handlers && i==WIFI_IF_STA && c->sta.threshold.authmode==WIFI_AUTH_WPA2_PSK && c->sta.pmf_cfg.capable);assert(!memcmp(c->sta.ssid,"synthetic",9) && !memcmp(c->sta.password,"test-only",9));return 0; }
esp_err_t esp_wifi_start(void) { assert(handlers && !started);started=true;++starts;return fail_start?1:0; }
esp_err_t esp_wifi_connect(void) { assert(started);return 0; }
esp_err_t esp_wifi_stop(void) { ++stops;if(fail_stop)return 1;started=linked=false;ni.up=false;return 0; }
esp_err_t esp_wifi_clear_default_wifi_driver_and_handlers(void *p) { assert(p==&ni && !started && allocated && attached);attached=handlers=false;return 0; }
void esp_netif_destroy(esp_netif_t *p) { assert(p==&ni && allocated && !attached);allocated=false; }
esp_err_t esp_wifi_deinit(void) { assert(initialized && !allocated && !started);initialized=false;++deinits;return 0; }
esp_err_t esp_wifi_sta_get_ap_info(wifi_ap_record_t *p) { assert(p && initialized);return linked?0:1; }
bool esp_netif_is_netif_up(esp_netif_t *p) { assert(p==&ni && allocated);return p->up; }
esp_err_t esp_netif_get_ip_info(esp_netif_t *p,esp_netif_ip_info_t *v) { assert(p==&ni && allocated && p->up);v->ip.addr=p->ip;return 0; }
static bool step(uint64_t t) { clock_ms=t;return room_wifi_idf_step(NULL); }
int main(int argc,char **argv) {
    assert(!room_wifi_idf_init((const uint8_t *)"x",1,(const uint8_t *)"short",5));
    assert(room_wifi_idf_init((const uint8_t *)"synthetic",9,(const uint8_t *)"test-only",9));
    assert(!room_wifi_idf_init((const uint8_t *)"synthetic",9,(const uint8_t *)"test-only",9));
    if(argc>1 && !strcmp(argv[1],"attach")) {
        fail_attach=true;assert(!step(0) && allocs==1 && !starts);assert(!step(100000) && allocs==1 && !starts);puts("PASS: ambiguous attach fails closed");return 0;
    }
    if(argc>1 && !strcmp(argv[1],"start")) {
        fail_start=true;assert(!step(0) && starts==1 && stops==1 && queued);deliver();assert(!step(1000) && deinits==1 && queued);deliver();fail_start=false;assert(!step(2000) && starts==2 && allocs==2);puts("PASS: partial start cleanup");return 0;
    }
    assert(!step(0) && starts==1 && allocs==1);linked=true;ni.up=true;assert(!step(1));ni.ip=42;assert(step(2));
    fail_stop=true;linked=false;assert(!step(3) && starts==1 && stops==1 && !queued);assert(!step(1002) && stops==1);fail_stop=false;fail_post=true;
    assert(!step(1003) && stops==2 && !queued);assert(!step(2003) && queued && starts==1);deliver();
    assert(!step(3003) && !allocated && deinits==1 && queued && starts==1);deliver();assert(!step(4003) && starts==2 && allocs==2 && ni.ip==0);
    linked=true;ni.up=true;assert(!step(4004));ni.ip=43;assert(step(4005));
    bool manual=false;down=true;clock_ms=4100;assert(room_wifi_idf_step(&manual)&&!manual);clock_ms=4150;assert(room_wifi_idf_step(&manual)&&!manual);clock_ms=7150;assert(room_wifi_idf_step(&manual)&&manual);clock_ms=8000;assert(room_wifi_idf_step(&manual)&&!manual);
    ni.ip=0;assert(!step(8001)&&queued);deliver();assert(!step(9001)&&queued);deliver();assert(!step(10001)&&starts==3);linked=true;ni.up=true;ni.ip=44;
    assert(!step(40001) && queued && !started);puts("PASS: RAM-only STA, fresh DHCP, loss, failed stop/post, two barriers, BOOT, late IP timeout");return 0;
}
