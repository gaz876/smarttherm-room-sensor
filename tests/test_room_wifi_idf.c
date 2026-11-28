#include "mock_wifi_sdk.h"
#include "room_wifi_idf.h"
#include "freertos/task.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint64_t clock_ms;
static int task_owner, other_task;
static TaskHandle_t current=&task_owner;
TaskHandle_t xTaskGetCurrentTaskHandle(void){return current;}
static bool initialized,allocated,attached,handlers,started,linked,down,fail_stop,fail_attach,fail_start,fail_post;
static unsigned starts,stops,allocs,deinits;
static bool stop_error_after,fail_deinit,deinit_error_after;
static unsigned deinit_calls;
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
esp_err_t esp_wifi_stop(void) { ++stops;if(fail_stop&&!stop_error_after)return 1;started=linked=false;ni.up=false;return fail_stop?1:0; }
esp_err_t esp_wifi_clear_default_wifi_driver_and_handlers(void *p) { assert(p==&ni && !started && allocated && attached);attached=handlers=false;return 0; }
void esp_netif_destroy(esp_netif_t *p) { assert(p==&ni && allocated && !attached);allocated=false; }
esp_err_t esp_wifi_deinit(void) { assert(initialized && !allocated && !started);++deinit_calls;if(fail_deinit&&!deinit_error_after)return 1;initialized=false;++deinits;return fail_deinit?1:0; }
esp_err_t esp_wifi_sta_get_ap_info(wifi_ap_record_t *p) { assert(p && initialized);return linked?0:1; }
bool esp_netif_is_netif_up(esp_netif_t *p) { assert(p==&ni && allocated);return p->up; }
esp_err_t esp_netif_get_ip_info(esp_netif_t *p,esp_netif_ip_info_t *v) { assert(p==&ni && allocated && p->up);v->ip.addr=p->ip;return 0; }
static bool step(uint64_t t) { clock_ms=t;return room_wifi_idf_step(NULL); }
static bool setup_ready=true,setup_running,setup_stoppable,setup_poll_ok=true,load_ok=true;
static unsigned setup_starts,setup_stops,profile_loads,binds;
static bool bind_setup(void *v){(void)v;assert(!initialized);++binds;return true;}
static bool ready_setup(void *v){(void)v;return setup_ready;}
static bool start_setup(void *v,uint64_t deadline,room_wifi_profile profile,bool physical){(void)v;assert(profile==ROOM_WIFI_PROFILE_VALID&&physical);assert(!initialized&&!allocated&&!started&&!queued&&!setup_running&&clock_ms<deadline);setup_running=true;++setup_starts;return true;}
static bool poll_setup(void *v){(void)v;assert(setup_running&&!initialized);return setup_poll_ok;}
static bool stop_setup(void *v,uint64_t deadline,room_wifi_setup_result *result){(void)v;assert(result&&setup_running&&!initialized&&clock_ms<deadline);++setup_stops;if(!setup_stoppable)return false;setup_running=false;*result=ROOM_WIFI_SETUP_NO_SAVE;return true;}
static bool load_profile(void *v,room_enroll_profile *p){(void)v;assert(!initialized&&!setup_running);++profile_loads;if(!load_ok)return false;*p=(room_enroll_profile){.ssid_len=9,.pass_len=9};memcpy(p->ssid,"synthetic",9);memcpy(p->pass,"test-only",9);return true;}
static int handoff(void){
    room_wifi_setup_io io={NULL,bind_setup,ready_setup,start_setup,poll_setup,stop_setup,load_profile};
    current=&other_task;assert(!room_wifi_idf_bind_setup(io)&&!step(0)&&!starts);
    current=&task_owner;assert(room_wifi_idf_bind_setup(io)&&binds==1&&!room_wifi_idf_bind_setup(io));
    assert(!step(0)&&starts==1&&profile_loads==1);linked=true;ni.up=true;ni.ip=1;assert(step(50));
    down=true;assert(step(100));assert(step(150));assert(!step(3150)&&queued&&stops==1&&!setup_starts);
    assert(!step(4149)&&!setup_starts);deliver();
    assert(!step(4150)&&queued&&deinits==1&&!setup_starts);deliver();
    assert(!step(5150)&&setup_running&&setup_starts==1&&!initialized);
    assert(!step(6000)&&setup_starts==1);
    setup_poll_ok=false;assert(!step(6050)&&setup_running&&setup_stops==1&&starts==1);
    assert(!step(7049)&&setup_stops==1);setup_stoppable=true;
    assert(!step(7050)&&!setup_running&&starts==2&&profile_loads==2);
    linked=true;ni.up=true;ni.ip=2;assert(step(7100));
    puts("PASS: real STA adapter, physical BOOT, two teardown barriers, exclusive setup callback, abort cleanup and protected reload");return 0;
}
int main(int argc,char **argv) {
    assert(!room_wifi_idf_init((const uint8_t *)"x",1,(const uint8_t *)"short",5));
    assert(room_wifi_idf_init((const uint8_t *)"synthetic",9,(const uint8_t *)"test-only",9));
    assert(!room_wifi_idf_init((const uint8_t *)"synthetic",9,(const uint8_t *)"test-only",9));
    if(argc>1 && !strcmp(argv[1],"handoff"))return handoff();
    if(argc>1 && !strcmp(argv[1],"attach")) {
        fail_attach=true;assert(!step(0) && allocs==1 && !starts);assert(!step(100000) && allocs==1 && !starts);puts("PASS: ambiguous attach fails closed");return 0;
    }
    if(argc>1 && !strcmp(argv[1],"start")) {
        fail_start=true;assert(!step(0) && starts==1 && stops==1 && queued);deliver();assert(!step(1000) && deinits==1 && queued);deliver();fail_start=false;assert(!step(2000) && starts==2 && allocs==2);puts("PASS: partial start cleanup");return 0;
    }
    if(argc>1 && (!strcmp(argv[1],"stop-error") || !strcmp(argv[1],"stop-error-after") ||
        !strcmp(argv[1],"deinit-error") || !strcmp(argv[1],"deinit-error-after"))) {
        bool stop_case=!strncmp(argv[1],"stop-",5);
        bool after=strstr(argv[1],"-after")!=NULL;
        assert(!step(0)&&starts==1);linked=true;ni.up=true;ni.ip=1;assert(step(50));
        fail_stop=stop_case;stop_error_after=after;fail_deinit=!stop_case;deinit_error_after=after;
        linked=false;assert(!step(100)&&stops==1);
        if(stop_case){assert(!queued&&allocated&&initialized&&started==!after);}
        else{assert(queued);deliver();assert(!step(1100)&&!queued&&!allocated&&deinit_calls==1&&initialized==!after);}
        fail_stop=fail_deinit=false;
        unsigned prior_stops=stops,prior_deinits=deinit_calls;
        assert(!step(2100)&&!step(10100)&&!step(100100));
        assert(stops==prior_stops&&deinit_calls==prior_deinits&&starts==1&&allocs==1);
        puts("PASS: ambiguous SDK cleanup error quarantines STA without retry");return 0;
    }
    assert(!step(0) && starts==1 && allocs==1);linked=true;ni.up=true;assert(!step(1));ni.ip=42;assert(step(2));
    linked=false;fail_post=true;assert(!step(3) && starts==1 && stops==1 && !queued);assert(!step(1002) && stops==1);
    assert(!step(1003) && stops==1 && queued);deliver();
    assert(!step(3003) && !allocated && deinits==1 && queued && starts==1);deliver();assert(!step(4003) && starts==2 && allocs==2 && ni.ip==0);
    linked=true;ni.up=true;assert(!step(4004));ni.ip=43;assert(step(4005));
    bool manual=false;down=true;clock_ms=4100;assert(room_wifi_idf_step(&manual)&&!manual);clock_ms=4150;assert(room_wifi_idf_step(&manual)&&!manual);clock_ms=7150;assert(room_wifi_idf_step(&manual)&&manual);clock_ms=8000;assert(room_wifi_idf_step(&manual)&&!manual);
    ni.ip=0;assert(!step(8001)&&queued);deliver();assert(!step(9001)&&queued);deliver();assert(!step(10001)&&starts==3);linked=true;ni.up=true;ni.ip=44;
    assert(!step(40001) && queued && !started);puts("PASS: RAM-only STA, fresh DHCP, loss, failed post, two barriers, BOOT, late IP timeout");return 0;
}
