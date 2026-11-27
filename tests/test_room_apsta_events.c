#define main apsta_fixture_main
#include "test_room_apsta_owner.c"
#undef main
#define main sec2_fixture_main
#include "test_room_sec2_transport.c"
#undef main
#include "room_apsta_events.h"
const char ip_base[]="ip",wifi_base[]="wifi";
static esp_event_handler_t handler[2];
static void *args[2];
static int handles[2];
static room_apsta_event records[2];
static unsigned count;
int esp_event_handler_instance_register(esp_event_base_t b,int32_t id,esp_event_handler_t h,void *v,esp_event_handler_instance_t *out) {
    unsigned i=b==IP_EVENT?0u:1u;assert(id==ESP_EVENT_ANY_ID&&!handler[i]);handler[i]=h;args[i]=v;*out=&handles[i];return ESP_OK;
}
int esp_event_handler_instance_unregister(esp_event_base_t b,int32_t id,esp_event_handler_instance_t h) {
    unsigned i=b==IP_EVENT?0u:1u;assert(id==ESP_EVENT_ANY_ID&&h==&handles[i]&&handler[i]);handler[i]=NULL;return ESP_OK;
}
int xQueueSend(QueueHandle_t q,const void *v,unsigned wait) { assert(q==records&&!wait);if(count==2)return 0;records[count++]=*(const room_apsta_event*)v;return pdPASS; }
int xQueueReceive(QueueHandle_t q,void *v,unsigned wait) { assert(q==records&&!wait);if(!count)return 0;*(room_apsta_event*)v=records[0];records[0]=records[1];count--;return pdPASS; }
unsigned uxQueueMessagesWaiting(QueueHandle_t q){assert(q==records);return count;}
int main(void) {
    room_apsta_events e={0};room_apsta_session s={0};
    assert(room_apsta_events_bind(&e,records));assert(!room_apsta_events_bind(&e,records));
    for(unsigned i=0;i<10;i++)handler[1](args[1],WIFI_EVENT,WIFI_EVENT_STA_DISCONNECTED,NULL);
    assert(!count&&!atomic_load(&e.overflow)&&!atomic_load(&e.accepting));
    assert(room_apsta_events_begin(&e)&&!room_apsta_events_begin(&e));
    s.radio.phase=APSTA_RUNNING;s.radio.trial=1;s.radio.sta=&sta;s.bridge.window=1;
    ip_event_got_ip_t got={.esp_netif=&ap,.ip_info={.ip={.addr=1}}};
    handler[0](args[0],IP_EVENT,IP_EVENT_STA_GOT_IP,&got);
    assert(room_apsta_events_poll(&e,&s)&&!s.lease);
    got.esp_netif=&sta;handler[0](args[0],IP_EVENT,IP_EVENT_STA_GOT_IP,&got);
    assert(!s.lease);assert(room_apsta_events_poll(&e,&s)&&s.lease==1);
    handler[1](args[1],WIFI_EVENT,WIFI_EVENT_STA_DISCONNECTED,NULL);
    assert(room_apsta_events_poll(&e,&s)&&s.bridge.stopping&&!s.lease);
    s.bridge.stopping=false;
    for(unsigned i=0;i<3;i++)handler[0](args[0],IP_EVENT,IP_EVENT_STA_GOT_IP,&got);
    assert(!room_apsta_events_poll(&e,&s)&&s.bridge.stopping&&!s.lease);
    assert(room_apsta_events_discard(&e)&&!count&&atomic_load(&e.overflow));
    assert(!room_apsta_events_poll(&e,&s)&&!s.lease);
    room_apsta_events_end(&e);
    assert(!room_apsta_events_begin(&e));
    for(unsigned i=0;i<10;i++)handler[1](args[1],WIFI_EVENT,WIFI_EVENT_STA_DISCONNECTED,NULL);
    assert(!count&&atomic_load(&e.overflow));
    assert(room_apsta_events_unbind(&e)&&!handler[0]&&!handler[1]);
    assert(!room_apsta_events_poll(&e,&s)&&!room_apsta_events_bind(&e,records));
    puts("APSTA SDK events mock PASS");return 0;
}
