#include "room_apsta_events.h"
#include "esp_wifi.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
int ip_base,wifi_base;
static struct {esp_event_handler_t fn;void *ctx;} wifi_cb,ip_cb;
static room_apsta_event queued;
static bool full;
static esp_netif_t ap={1},other={2};
static const wifi_event_ap_staconnected_t station={{2,1,2,3,4,5},1,false};
static const uint8_t wrong[6]={2,1,2,3,4,6};
int esp_event_handler_instance_register(esp_event_base_t b,int32_t id,esp_event_handler_t f,void *ctx,esp_event_handler_instance_t *out){
    assert(id==ESP_EVENT_ANY_ID&&f&&ctx&&out);
    if(b==WIFI_EVENT){wifi_cb.fn=f;wifi_cb.ctx=ctx;*out=&wifi_cb;}
    else {assert(b==IP_EVENT);ip_cb.fn=f;ip_cb.ctx=ctx;*out=&ip_cb;}return ESP_OK;
}
int esp_event_handler_instance_unregister(esp_event_base_t b,int32_t id,esp_event_handler_instance_t h){
    assert(id==ESP_EVENT_ANY_ID&&h);if(b==WIFI_EVENT)wifi_cb.fn=NULL;else ip_cb.fn=NULL;return ESP_OK;
}
int xQueueSend(QueueHandle_t q,const void *p,unsigned wait){assert(q&&!wait);if(full)return 0;queued=*(const room_apsta_event *)p;full=true;return pdPASS;}
int xQueueReceive(QueueHandle_t q,void *p,unsigned wait){assert(q&&!wait);if(!full)return 0;*(room_apsta_event *)p=queued;full=false;return pdPASS;}
unsigned uxQueueMessagesWaiting(QueueHandle_t q){assert(q);return full;}
void room_apsta_session_lost(room_apsta_session *s){++s->lost;}
void room_apsta_session_ip(room_apsta_session *s,const esp_netif_t *n,uint32_t ip){assert(n&&ip);++s->ip;}
static void emit(int32_t id,const void *p){assert(wifi_cb.fn);wifi_cb.fn(wifi_cb.ctx,WIFI_EVENT,id,(void *)p);}
static uint64_t epoch(room_apsta_events *s,const esp_netif_t *n,const uint8_t *m){uint64_t v=UINT64_MAX;bool ok=room_apsta_events_peer_epoch(s,n,m,&v);assert(ok==(v!=0));return v;}
static void begin(room_apsta_events *s){assert(room_apsta_events_begin(s));assert(room_apsta_events_peer_attach(s,&ap));assert(!epoch(s,&ap,station.mac));}
static void associate(void){emit(WIFI_EVENT_AP_START,NULL);emit(WIFI_EVENT_AP_STACONNECTED,&station);}
static void end(room_apsta_events *s){room_apsta_events_end(s);assert(!epoch(s,&ap,station.mac));}
static void *reader(void *arg){room_apsta_events *s=arg;for(unsigned i=0;i<100000;++i){uint64_t a=epoch(s,&ap,station.mac);if(a)assert(a>0);assert(!epoch(s,&other,station.mac));}return NULL;}
int main(void){room_apsta_events s={0};int q;room_apsta_session session={0};assert(room_apsta_events_bind(&s,&q));
    assert(!epoch(&s,&ap,station.mac));associate();assert(!epoch(&s,&ap,station.mac));
    uint64_t last=0;
    for(unsigned i=0;i<256;++i){begin(&s);associate();uint64_t v=epoch(&s,&ap,station.mac);assert(v>last);last=v;
        assert(!epoch(&s,&other,station.mac)&&!epoch(&s,&ap,wrong));
        emit(WIFI_EVENT_AP_STADISCONNECTED,NULL);assert(!epoch(&s,&ap,station.mac));
        emit(WIFI_EVENT_AP_STACONNECTED,&station);assert(!epoch(&s,&ap,station.mac));end(&s);}
    begin(&s);emit(WIFI_EVENT_AP_STACONNECTED,&station);assert(!epoch(&s,&ap,station.mac));associate();assert(!epoch(&s,&ap,station.mac));end(&s);
    begin(&s);emit(WIFI_EVENT_AP_START,NULL);emit(WIFI_EVENT_AP_STACONNECTED,NULL);assert(!epoch(&s,&ap,station.mac));end(&s);
    begin(&s);emit(WIFI_EVENT_AP_START,NULL);wifi_event_ap_staconnected_t invalid=station;invalid.aid=0;emit(WIFI_EVENT_AP_STACONNECTED,&invalid);assert(!epoch(&s,&ap,station.mac));end(&s);
    begin(&s);associate();emit(WIFI_EVENT_AP_STACONNECTED,&station);assert(!epoch(&s,&ap,station.mac));end(&s);
    begin(&s);associate();emit(WIFI_EVENT_AP_STOP,NULL);assert(!epoch(&s,&ap,station.mac));end(&s);
    begin(&s);associate();room_apsta_events_peer_detach(&s);assert(!epoch(&s,&ap,station.mac));assert(!room_apsta_events_peer_attach(&s,&ap));end(&s);
    assert(room_apsta_events_begin(&s));associate();assert(!epoch(&s,&ap,station.mac));assert(room_apsta_events_peer_attach(&s,&ap));assert(epoch(&s,&ap,station.mac)>last);end(&s);
    begin(&s);associate();pthread_t t;assert(!pthread_create(&t,NULL,reader,&s));emit(WIFI_EVENT_AP_STADISCONNECTED,NULL);assert(!epoch(&s,&ap,station.mac));assert(!pthread_join(t,NULL));end(&s);
    begin(&s);associate();ip_event_got_ip_t got={.esp_netif=&other,.ip_info={.ip={.addr=1}}};ip_cb.fn(ip_cb.ctx,IP_EVENT,IP_EVENT_STA_GOT_IP,&got);assert(room_apsta_events_poll(&s,&session)&&session.ip==1);emit(WIFI_EVENT_STA_DISCONNECTED,NULL);assert(room_apsta_events_poll(&s,&session)&&session.lost==1);
    emit(WIFI_EVENT_STA_DISCONNECTED,NULL);emit(WIFI_EVENT_STA_DISCONNECTED,NULL);assert(!epoch(&s,&ap,station.mac));assert(!room_apsta_events_poll(&s,&session));end(&s);assert(!room_apsta_events_begin(&s));assert(room_apsta_events_unbind(&s));assert(!pthread_mutex_destroy(&s.peer_lock));
    puts("PASS: 256 AP windows; monotonically distinct associations; invalid, duplicate and stop events; detach; independent reader; STA queue/overflow");return 0;}
