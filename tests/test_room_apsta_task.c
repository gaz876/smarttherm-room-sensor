#define main apsta_fixture_main
#include "test_room_apsta_owner.c"
#undef main
#define main sec2_fixture_main
#include "test_room_sec2_transport.c"
#undef main
#include "room_apsta_task.h"
const char ip_base[]="ip",wifi_base[]="wifi";
static esp_event_handler_t handlers[3];
static void *args[3];
static int handles[3],owner_id,other_id;
static TaskHandle_t current=&owner_id;
static int64_t us;
static room_apsta_event records[8];
static unsigned count;
static uint32_t posted;
static esp_event_base_t barrier_base;
TaskHandle_t xTaskGetCurrentTaskHandle(void){return current;}
int64_t esp_timer_get_time(void){return us;}
int esp_event_handler_instance_register(esp_event_base_t b,int32_t id,esp_event_handler_t h,void *v,esp_event_handler_instance_t *out){
    unsigned i=b==IP_EVENT?0u:b==WIFI_EVENT?1u:2u;
    assert(!handlers[i]&&id==(i==2?0:ESP_EVENT_ANY_ID));handlers[i]=h;args[i]=v;*out=&handles[i];
    if(i==2)barrier_base=b;
    return ESP_OK;
}
int esp_event_handler_instance_unregister(esp_event_base_t b,int32_t id,esp_event_handler_instance_t h){
    unsigned i=b==IP_EVENT?0u:b==WIFI_EVENT?1u:2u;
    assert(id==(i==2?0:ESP_EVENT_ANY_ID)&&h==&handles[i]);handlers[i]=NULL;return ESP_OK;
}
int esp_event_post(esp_event_base_t b,int32_t id,const void *v,size_t n,unsigned wait){
    assert(b==barrier_base&&!id&&n==sizeof(posted)&&!wait&&!posted);memcpy(&posted,v,n);return ESP_OK;
}
static void deliver_barrier(void){assert(posted);uint32_t token=posted;posted=0;handlers[2](args[2],barrier_base,0,&token);}
int xQueueSend(QueueHandle_t q,const void *v,unsigned wait){assert(q==records&&!wait);if(count==8)return 0;records[count++]=*(const room_apsta_event*)v;return pdPASS;}
int xQueueReceive(QueueHandle_t q,void *v,unsigned wait){assert(q==records&&!wait);if(!count)return 0;*(room_apsta_event*)v=records[0];memmove(records,records+1,(--count)*sizeof(records[0]));return pdPASS;}
unsigned uxQueueMessagesWaiting(QueueHandle_t q){assert(q==records);return count;}
int main(void){
    room_apsta_task t={0};fake f={0};room_enroll_store store={&f,ready,write_record,read_record};
    const char salt[16]={0},verifier[384]={0};protocomm_security2_params_t id={salt,16,verifier,384};
    const uint8_t *out;size_t n;uint8_t hs[]={1},stage[29]={1,ROOM_SETUP_STAGE,1,8,'x','a','b','c','d','e','f','g','h'},save[26]={1,ROOM_SETUP_SAVE,0,0,0,0,0,0,0,1};
    reset();assert(room_apsta_task_bind(&t,records));assert(!room_apsta_task_bind(&t,records));
    current=&other_id;assert(!room_apsta_task_open(&t,PHONE_PROFILE_MISSING,false,true,&store,ssid,1,key,16,&id,7));assert(!calls);
    current=&owner_id;assert(!room_apsta_task_open(&t,PHONE_PROFILE_MISSING,false,false,&store,ssid,1,key,16,&id,7));
    assert(room_apsta_task_open(&t,PHONE_PROFILE_MISSING,false,true,&store,ssid,1,key,16,&id,7));
    current=&other_id;assert(!room_apsta_task_poll(&t));assert(!room_apsta_task_packet(&t,true,hs,sizeof(hs),&out,&n));assert(!out&&!n);assert(!room_apsta_task_close(&t,true));assert(!stops);
    current=&owner_id;assert(room_apsta_task_packet(&t,true,hs,sizeof(hs),&out,&n));assert(room_apsta_task_sent(&t,true));
    us=1000;assert(room_apsta_task_packet(&t,false,stage,sizeof(stage),&out,&n));assert(room_apsta_task_sent(&t,true));
    assert(room_apsta_task_poll(&t)&&connects==1);
    linked=true;sta.up=true;sta.ip=1;
    ip_event_got_ip_t got={.esp_netif=&ap,.ip_info={.ip={.addr=1}}};
    handlers[0](args[0],IP_EVENT,IP_EVENT_STA_GOT_IP,&got);assert(room_apsta_task_poll(&t)&&!t.session.lease);
    got.esp_netif=&sta;handlers[0](args[0],IP_EVENT,IP_EVENT_STA_GOT_IP,&got);
    assert(room_apsta_task_poll(&t)&&t.session.bridge.phone.enrollment.state==ENROLL_VERIFIED);
    us=101000;assert(room_apsta_task_packet(&t,false,save,sizeof(save),&out,&n)&&out[1]==ROOM_REPLY_SAVED&&f.writes==1);
    assert(room_apsta_task_sent(&t,true));assert(!room_apsta_task_close(&t,false)&&!stops&&!posted);
    assert(!room_apsta_task_close(&t,true)&&stops==1&&posted&&!destroys);
    assert(!room_apsta_task_close(&t,true)&&stops==1&&!destroys);pre=true;deliver_barrier();
    assert(!room_apsta_task_close(&t,true)&&destroys==2&&deinits==1&&posted&&t.session.bridge.window);
    post=true;deliver_barrier();assert(room_apsta_task_close(&t,true)&&!t.session.bridge.window&&!t.pending);
    reset();us=200000;assert(room_apsta_task_open(&t,PHONE_PROFILE_VALID,true,true,&store,ssid,1,key,16,&id,7));
    handlers[1](args[1],WIFI_EVENT,WIFI_EVENT_STA_DISCONNECTED,NULL);assert(!room_apsta_task_poll(&t));
    assert(!room_apsta_task_close(&t,true)&&posted);us=5200000;assert(!room_apsta_task_close(&t,true)&&t.session.locked);
    /* Fake allocation only: the production context remains quarantined. */
    protocomm_delete(t.session.transport.sec.pc);
    puts("APSTA network task mock PASS");return 0;
}
