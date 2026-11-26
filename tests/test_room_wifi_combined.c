/* Actual STA + APSTA link + APSTA task/session on one mock SDK driver.
 * No real keys, cryptography, phone listener, or network traffic. */
#define main sec2_fixture_main
#include "test_room_sec2_transport.c"
#undef main
#include "room_wifi_apsta_link.h"
#include "room_wifi_secure_store.h"
#include "driver/gpio.h"
#include "esp_timer.h"

struct esp_netif { unsigned type; bool allocated,attached,up; uint32_t ip; };
static struct esp_netif ap={.type=1},sta={.type=2};
static wifi_mode_t mode;
static bool driver,started,linked,button,pre,provider_ok=true,prepare_ok=true,fail_ap_start;
static unsigned sta_starts,ap_starts,stops,deinits,loads,prepares,connects;
static uint64_t ms;
static int owner_id,other_id;
static TaskHandle_t current=&owner_id;
static fake storage;
static room_secure_status storage_status=ROOM_SEC_VALID;
static bool network_inited,loop_inited;
const char ip_base[]="ip",wifi_base[]="wifi";
static struct { esp_event_base_t base; esp_event_handler_t fn; void *ctx; int32_t id; } callbacks[4];
static unsigned registered;
static esp_event_base_t posted_base;
static uint32_t posted;
static room_apsta_event *queue;
static unsigned count;
static bool queue_created;

TaskHandle_t xTaskGetCurrentTaskHandle(void){return current;}
int64_t esp_timer_get_time(void){return (int64_t)ms*1000;}
int gpio_config(const gpio_config_t *c){assert(c->pin_bit_mask==(UINT64_C(1)<<9)&&c->mode==GPIO_MODE_INPUT&&c->pull_up_en);return ESP_OK;}
int gpio_get_level(int pin){assert(pin==9);return !button;}
int esp_netif_init(void){assert(!network_inited);network_inited=true;return ESP_OK;}
int esp_event_loop_create_default(void){assert(network_inited&&!loop_inited);loop_inited=true;return ESP_OK;}
static int register_cb(esp_event_base_t b,int32_t id,esp_event_handler_t fn,void *ctx){
    assert(loop_inited&&registered<4&&fn);
    for(unsigned i=0;i<registered;i++)assert(callbacks[i].base!=b);
    unsigned i=registered++;callbacks[i].base=b;callbacks[i].fn=fn;callbacks[i].ctx=ctx;callbacks[i].id=id;return (int)i;
}
int esp_event_handler_register(esp_event_base_t b,int32_t id,esp_event_handler_t fn,void *ctx){
    assert(!registered&&id==0&&!ctx);(void)register_cb(b,id,fn,ctx);return ESP_OK;
}
int esp_event_handler_instance_register(esp_event_base_t b,int32_t id,esp_event_handler_t fn,void *ctx,esp_event_handler_instance_t *out){
    unsigned i=(unsigned)register_cb(b,id,fn,ctx);*out=&callbacks[i];return ESP_OK;
}
int esp_event_handler_instance_unregister(esp_event_base_t b,int32_t id,esp_event_handler_instance_t h){
    (void)b;(void)id;(void)h;assert(0&&"persistent callbacks must not be unregistered");return ESP_FAIL;
}
int esp_event_post(esp_event_base_t b,int32_t id,const void *p,size_t n,unsigned wait){
    assert(!posted&&!id&&n==sizeof(posted)&&!wait);posted_base=b;memcpy(&posted,p,n);assert(posted);return ESP_OK;
}
static void deliver(void){
    assert(posted);uint32_t token=posted;posted=0;
    for(unsigned i=0;i<registered;i++)if(callbacks[i].base==posted_base){
        callbacks[i].fn(callbacks[i].ctx,posted_base,0,&token);pre=true;return;
    }
    assert(0&&"unknown barrier");
}
static void disconnect_event(void){
    for(unsigned i=0;i<registered;i++)if(callbacks[i].base==WIFI_EVENT)
        callbacks[i].fn(callbacks[i].ctx,WIFI_EVENT,WIFI_EVENT_STA_DISCONNECTED,NULL);
}
QueueHandle_t xQueueCreateStatic(unsigned depth,unsigned item,uint8_t *buf,StaticQueue_t *control){
    assert(!queue_created&&depth==ROOM_APSTA_QUEUE_DEPTH&&item==sizeof(room_apsta_event)&&buf&&control);
    queue_created=true;queue=(room_apsta_event *)buf;return queue;
}
int xQueueSend(QueueHandle_t q,const void *v,unsigned wait){assert(q==queue&&!wait);if(count==ROOM_APSTA_QUEUE_DEPTH)return 0;queue[count++]=*(const room_apsta_event*)v;return pdPASS;}
int xQueueReceive(QueueHandle_t q,void *v,unsigned wait){assert(q==queue&&!wait);if(!count)return 0;*(room_apsta_event*)v=queue[0];memmove(queue,queue+1,(--count)*sizeof(*queue));return pdPASS;}
unsigned uxQueueMessagesWaiting(QueueHandle_t q){assert(q==queue);return count;}

int esp_wifi_init(const wifi_init_config_t *c){
    assert(!driver&&!started&&!ap.allocated&&!sta.allocated&&!posted&&!c->nvs_enable);
    driver=true;pre=false;mode=WIFI_MODE_NULL;return ESP_OK;
}
int esp_wifi_set_storage(wifi_storage_t s){assert(driver&&!started&&s==WIFI_STORAGE_RAM);return ESP_OK;}
int esp_wifi_set_mode(wifi_mode_t m){assert(driver&&!started&&(m==WIFI_MODE_STA||m==WIFI_MODE_APSTA));mode=m;return ESP_OK;}
esp_netif_t *esp_netif_new(const esp_netif_config_t *c){
    assert(driver&&!started);struct esp_netif *p=c->type==1?&ap:&sta;assert(!p->allocated);
    *p=(struct esp_netif){.type=c->type,.allocated=true};return p;
}
int esp_netif_attach_wifi_ap(esp_netif_t *p){assert(p==&ap&&p->allocated&&!p->attached);p->attached=true;return ESP_OK;}
int esp_netif_attach_wifi_station(esp_netif_t *p){assert(p==&sta&&p->allocated&&!p->attached);p->attached=true;return ESP_OK;}
int esp_wifi_set_default_wifi_sta_handlers(void){assert(sta.attached&&!ap.allocated&&!started);return ESP_OK;}
int esp_wifi_set_default_wifi_ap_handlers(void){assert(ap.attached&&sta.attached&&!started);return ESP_OK;}
int esp_wifi_set_config(wifi_interface_t i,wifi_config_t *c){
    assert(driver);
    if(i==WIFI_IF_AP){assert(mode==WIFI_MODE_APSTA&&!started&&c->ap.authmode==WIFI_AUTH_WPA2_PSK&&c->ap.max_connection==1&&c->ap.ssid_len==4);}
    else {assert(c->sta.threshold.authmode==WIFI_AUTH_WPA2_PSK&&c->sta.pmf_cfg.capable);if(mode==WIFI_MODE_STA){assert(!started&&!memcmp(c->sta.ssid,storage.saved.ssid,storage.saved.ssid_len));assert(!memcmp(c->sta.password,storage.saved.pass,storage.saved.pass_len));}}
    return ESP_OK;
}
int esp_wifi_start(void){
    assert(driver&&!started&&sta.attached);started=true;
    if(mode==WIFI_MODE_APSTA){assert(ap.attached);++ap_starts;ap.up=true;return fail_ap_start?ESP_FAIL:ESP_OK;}
    assert(mode==WIFI_MODE_STA&&!ap.allocated);++sta_starts;return ESP_OK;
}
int esp_wifi_connect(void){assert(driver&&started&&sta.attached);++connects;return ESP_OK;}
int esp_wifi_sta_get_ap_info(wifi_ap_record_t *p){assert(p&&driver);return linked?ESP_OK:ESP_FAIL;}
bool esp_netif_is_netif_up(esp_netif_t *p){assert(p->allocated);return p->up;}
int esp_netif_get_ip_info(esp_netif_t *p,esp_netif_ip_info_t *ip){assert(p->allocated);ip->ip.addr=p->ip;return ESP_OK;}
int esp_netif_dhcpc_get_status(esp_netif_t *p,esp_netif_dhcp_status_t *s){assert(p==&sta&&p->allocated);*s=ESP_NETIF_DHCP_STARTED;return ESP_OK;}
int esp_wifi_stop(void){assert(driver&&started);++stops;started=linked=pre=false;ap.up=sta.up=false;return ESP_OK;}
int esp_wifi_clear_default_wifi_driver_and_handlers(void *v){struct esp_netif *p=v;assert(pre&&!posted&&!started&&p->allocated&&p->attached&&!p->up);p->attached=false;return ESP_OK;}
void esp_netif_destroy(esp_netif_t *p){assert(pre&&!posted&&!p->attached&&!p->up&&p->allocated);p->allocated=false;}
int esp_wifi_deinit(void){assert(driver&&!started&&!posted&&!ap.allocated&&!sta.allocated&&pre);driver=false;++deinits;return ESP_OK;}

room_enroll_store room_secure_store(void){return (room_enroll_store){&storage,ready,write_record,read_record};}
room_secure_status room_secure_load(room_enroll_profile *p){assert(!driver&&!posted);++loads;*p=storage_status==ROOM_SEC_VALID?storage.saved:(room_enroll_profile){0};return storage_status;}
static bool provider_ready(void *v){assert(v==&storage);return provider_ok;}
static bool provider_prepare(void *v,room_wifi_apsta_window *p){
    static const char salt[16]={0},verifier[384]={0};
    static const protocomm_security2_params_t id={salt,16,verifier,384};
    assert(v==&storage&&!driver&&!posted);++prepares;
    if(!prepare_ok)return false;
    memcpy(p->ssid,"mock",4);p->ssid_len=4;
    memcpy(p->key,"test-only-key-16",16);p->key_len=16;p->identity=&id;p->sdk_id=7;return true;
}
static bool tick(uint64_t now){ms=now;return room_wifi_idf_step(NULL);}
static void online(uint64_t now){linked=true;sta.up=true;sta.ip=1;assert(tick(now));}
static int first_boot(const char *scenario) {
    room_wifi_apsta_provider p={&storage,provider_ready,provider_prepare};
    if(!strcmp(scenario,"first_error"))storage_status=ROOM_SEC_ERROR;
    else if(!strcmp(scenario,"first_unavailable"))storage_status=ROOM_SEC_UNAVAILABLE;
    else if(!strcmp(scenario,"first_valid"))storage_status=ROOM_SEC_VALID;
    else storage_status=ROOM_SEC_MISSING;
    if(storage_status!=ROOM_SEC_MISSING){
        assert(!room_wifi_apsta_link_init_missing(p)&&loads==1&&!network_inited&&!driver&&!prepares);
        puts("PASS first boot refuses nonmissing protected profile");return 0;
    }
    if(!strcmp(scenario,"first_late"))provider_ok=false;
    assert(room_wifi_apsta_link_init_missing(p)&&loads==1&&registered==4&&!driver);
    if(!strcmp(scenario,"first_changed"))storage_status=ROOM_SEC_ERROR;
    assert(!tick(0)&&!sta_starts&&!connects);
    if(storage_status!=ROOM_SEC_MISSING){
        assert(!ap_starts&&!driver&&!prepares&&loads==2);
        assert(!tick(300000)&&!ap_starts);puts("PASS first boot rechecks protected profile");return 0;
    }
    uint64_t opened=0;
    if(!provider_ok){
        assert(!driver&&!ap_starts&&loads==1);
        provider_ok=true;assert(!tick(50)&&!ap_starts&&!prepares);
        button=true;assert(!tick(100));assert(!tick(150));assert(!tick(3150));opened=3150;
    }
    assert(ap_starts==1&&driver&&mode==WIFI_MODE_APSTA&&prepares==1&&loads==2&&!connects);
    uint64_t end=opened+300000;
    assert(!tick(end)&&posted&&stops==1&&!started);
    deliver();assert(!tick(end+1000)&&posted&&!driver&&deinits==1);
    deliver();assert(!tick(end+2000)&&!driver&&!sta_starts&&!posted);
    assert(!tick(end+5000)&&ap_starts==1&&!driver&&prepares==1);
    /* Manual gesture is the only way to reopen after timeout. */
    button=false;assert(!tick(end+5100));assert(!tick(end+5150));
    button=true;assert(!tick(end+5200));assert(!tick(end+5250));assert(!tick(end+8250)&&ap_starts==2&&prepares==2);
    disconnect_event();assert(!tick(end+8300)&&posted&&stops==2);
    deliver();assert(!tick(end+9300)&&posted&&!driver);
    deliver();assert(!tick(end+10300)&&!driver&&!sta_starts&&!posted);
    puts("PASS first boot window and manual-only reopen");return 0;
}
int main(int argc,char **argv){
    const char *scenario=argc>1?argv[1]:"normal";
    storage.saved=(room_enroll_profile){.ssid_len=4,.pass_len=8,.ssid={'h','o','m','e'},.pass={'t','e','s','t','o','n','l','y'}};
    if(!strncmp(scenario,"first_",6))return first_boot(scenario);
    assert(room_wifi_idf_init(storage.saved.ssid,4,storage.saved.pass,8));
    room_wifi_apsta_provider p={&storage,provider_ready,provider_prepare};
    current=&other_id;assert(!room_wifi_apsta_link_bind(p)&&!tick(0));
    current=&owner_id;assert(room_wifi_apsta_link_bind(p)&&registered==4&&queue_created);
    assert(!room_wifi_apsta_link_bind(p));
    assert(!tick(0)&&sta_starts==1&&loads==1&&connects==1);online(50);
    for(unsigned i=0;i<20;i++)disconnect_event();
    assert(!count);
    if(!strcmp(scenario,"notready"))provider_ok=false;
    if(!strcmp(scenario,"prepare"))prepare_ok=false;
    if(!strcmp(scenario,"partial"))fail_ap_start=true;
    button=true;assert(tick(100));assert(tick(150));
    if(!provider_ok){
        assert(tick(3150)&&!stops&&!prepares&&!ap_starts);puts("PASS combined unavailable provider");return 0;
    }
    assert(!tick(3150)&&posted&&stops==1&&!prepares&&!ap_starts);
    if(!strcmp(scenario,"timeout")){
        assert(!tick(8150)&&!tick(20000)&&!ap_starts&&sta_starts==1&&!prepares);
        puts("PASS combined stalled STA cleanup quarantines owner");return 0;
    }
    deliver();assert(!tick(4150)&&posted&&!driver&&deinits==1&&!prepares);
    deliver();assert(!tick(5150)&&prepares==1);
    if(!prepare_ok){
        assert(!driver&&!posted&&!ap_starts);assert(!tick(6150)&&sta_starts==2&&loads==2);online(6200);
        puts("PASS combined rejected provider prepare returns to STA");return 0;
    }
    if(fail_ap_start){
        assert(ap_starts==1&&posted&&stops==2&&!started);deliver();
        assert(!tick(6150)&&posted&&!driver&&deinits==2&&sta_starts==1);deliver();
        assert(!tick(7150)&&sta_starts==2&&loads==2&&!ap.allocated);online(7200);
        puts("PASS combined partial AP start drains before STA");return 0;
    }
    assert(driver&&started&&mode==WIFI_MODE_APSTA&&ap_starts==1&&sta_starts==1&&connects==1&&!posted);
    assert(!tick(5200)&&stops==1); /* STA was stopped exactly once. */
    uint64_t close_at=5300;
    if(!strcmp(scenario,"expiry"))close_at=303150;
    else if(!strcmp(scenario,"revoked"))provider_ok=false;
    else disconnect_event();
    assert(!tick(close_at)&&posted&&stops==2&&!started&&sta_starts==1);
    deliver();assert(!tick(close_at+1000)&&posted&&!driver&&deinits==2&&sta_starts==1);
    deliver();
    /* Simulate a different valid protected profile on next STA start. */
    storage.saved.ssid[0]='n';assert(!tick(close_at+2000)&&sta_starts==2&&loads==2&&connects==2&&!ap.allocated);
    online(close_at+2050);
    for(unsigned i=0;i<20;i++)disconnect_event();
    assert(!count&&sta_starts==2&&ap_starts==1);
    puts("PASS combined actual STA/APSTA adapters, exclusive driver, both barriers, protected reload");return 0;
}
