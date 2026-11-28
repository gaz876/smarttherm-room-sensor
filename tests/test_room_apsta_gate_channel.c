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
static room_apsta_event records[ROOM_APSTA_QUEUE_DEPTH];
static bool fail_queue;
static unsigned queue_creates;
static room_apsta_task *expected_queue_owner;
static unsigned count;
static uint32_t posted;
static bool fail_post;
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
    assert(b==barrier_base&&!id&&n==sizeof(posted)&&!wait&&!posted);
    if(fail_post)return ESP_FAIL;
    memcpy(&posted,v,n);return ESP_OK;
}
static void deliver_barrier(void){assert(posted);uint32_t token=posted;posted=0;handlers[2](args[2],barrier_base,0,&token);}
QueueHandle_t xQueueCreateStatic(unsigned depth,unsigned item,uint8_t *storage,StaticQueue_t *control){
    assert(expected_queue_owner && depth==ROOM_APSTA_QUEUE_DEPTH && item==sizeof(room_apsta_event));
    assert(storage==(uint8_t *)expected_queue_owner->queue_storage && control==&expected_queue_owner->queue_control);
    ++queue_creates;return fail_queue?NULL:records;
}
int xQueueSend(QueueHandle_t q,const void *v,unsigned wait){assert(q==records&&!wait);if(count==ROOM_APSTA_QUEUE_DEPTH)return 0;records[count++]=*(const room_apsta_event*)v;return pdPASS;}
int xQueueReceive(QueueHandle_t q,void *v,unsigned wait){assert(q==records&&!wait);if(!count)return 0;*(room_apsta_event*)v=records[0];memmove(records,records+1,(--count)*sizeof(records[0]));return pdPASS;}
unsigned uxQueueMessagesWaiting(QueueHandle_t q){assert(q==records);return count;}
#include "room_phone_channel.h"
#include "stop_gate_adapter.h"

/* Isolated owner integration: actual task/session/channel/transport sources,
 * only synthetic SDK radio, event loop and security transport mocks.
 * All generation IDs are local. No network, credentials or hardware. */
typedef struct {
    stop_gate_adapter gate;
    uint64_t window,ticket;
    bool received,published,exited,drained;
} channel_fixture;
static bool gate_clock(void *ctx,uint64_t *now){
    (void)ctx;if(us<0)return false;*now=(uint64_t)us/1000u;return true;
}
static ack_result ev(channel_fixture *f,mb_event e,bool receipt,mb_reply *out){
    e.window=f->window;return gate_step(&f->gate,e,receipt,out,gate_clock,NULL);
}
static bool io_open(void *ctx,const esp_netif_t *netif,uint64_t w){
    channel_fixture *f=ctx;f->window=w;f->ticket=0;f->received=f->published=f->exited=f->drained=false;
    return netif==&ap&&gate_open(&f->gate,w,gate_clock,NULL);
}
static room_phone_io_result io_recv(void *ctx,bool *hs,uint8_t *p,size_t cap,size_t *n){
    channel_fixture *f=ctx;mb_reply out;*n=0;
    if(!f->ticket||f->received)return ROOM_PHONE_IO_WAIT;
    ack_result rc=ev(f,(mb_event){.action=MB_TAKE_REQUEST,.ticket=f->ticket,.output=p,.size=cap},false,&out);
    if(rc==ACK_BUSY)return ROOM_PHONE_IO_WAIT;
    if(rc!=ACK_OK)return ROOM_PHONE_IO_ERROR;
    f->received=true;*hs=out.handshake;*n=out.length;return ROOM_PHONE_IO_OK;
}
static room_phone_io_result io_send(void *ctx,const uint8_t *p,size_t n,size_t *sent){
    channel_fixture *f=ctx;mb_reply out;*sent=0;
    if(!f->published){
        ack_result rc=ev(f,(mb_event){.action=MB_PUBLISH,.ticket=f->ticket,.input=p,.size=n},false,&out);
        if(rc==ACK_BUSY)return ROOM_PHONE_IO_WAIT;
        if(rc!=ACK_OK)return ROOM_PHONE_IO_ERROR;
        f->published=true;return ROOM_PHONE_IO_WAIT;
    }
    ack_result rc=ev(f,(mb_event){.ticket=f->ticket},true,&out);
    if(rc==ACK_WAIT||rc==ACK_BUSY)return ROOM_PHONE_IO_WAIT;
    if(rc!=ACK_OK||out.ticket!=f->ticket)return ROOM_PHONE_IO_ERROR;
    *sent=n;return ROOM_PHONE_IO_OK;
}
static bool no_legacy(void *ctx){(void)ctx;assert(false);return false;}
static bool timed_close(void *ctx,uint64_t w,uint64_t deadline){
    channel_fixture *f=ctx;return gate_close_until(&f->gate,w,deadline,f->exited,f->drained,gate_clock,NULL)==ACK_OK;
}
static void begin_packet(channel_fixture *f,room_phone_channel *c,const uint8_t *p,size_t n,bool hs,uint8_t expected){
    mb_reply out;uint8_t copy[4096];uint64_t previous=f->ticket;
    f->received=f->published=false;
    assert(ev(f,(mb_event){.action=MB_SUBMIT,.input=p,.size=n,.flag=hs},false,&out)==ACK_OK);
    f->ticket=out.ticket;assert(f->ticket>previous);
    assert(room_phone_channel_poll(c)&&c->task->session.transport.pending&&c->reply);
    assert(room_phone_channel_poll(c)&&f->published&&!c->offset);
    assert(ev(f,(mb_event){.action=MB_TAKE_REPLY,.ticket=f->ticket,.output=copy,.size=sizeof copy},false,&out)==ACK_OK);
    assert(out.length==(hs?1u:27u)&&copy[hs?0:1]==expected);
    assert(room_phone_channel_poll(c)&&c->reply&&c->task->session.transport.pending);
}
static void confirm_packet(channel_fixture *f,room_phone_channel *c){
    mb_reply out;
    assert(ev(f,(mb_event){.action=MB_SENT,.ticket=f->ticket,.flag=true},false,&out)==ACK_OK);
    assert(room_phone_channel_poll(c)&&!c->reply&&!c->task->session.transport.pending);
    assert(ev(f,(mb_event){.ticket=f->ticket},true,&out)==ACK_REJECT);
}
static void finish_radio(room_apsta_task *t,uint64_t deadline){
    assert(!room_apsta_task_close_until(t,true,deadline)&&posted&&stops==1&&!destroys);
    pre=true;deliver_barrier();
    assert(!room_apsta_task_close_until(t,true,deadline)&&posted&&destroys==2&&deinits==1);
    post=true;deliver_barrier();
    assert(room_apsta_task_close_until(t,true,deadline)&&!t->session.bridge.window&&!t->pending&&!driver);
    assert(!atomic_load(&t->events.accepting)&&!t->session.transport.sec.pc);
}
int main(void){
    room_apsta_task t={0};room_phone_channel c={0};channel_fixture f={0};
    fake state={0};room_enroll_store store={&state,ready,write_record,read_record};
    const char salt[16]={0},verifier[384]={0};protocomm_security2_params_t id={salt,16,verifier,384};
    const uint8_t hs[]={1},status[18]={1,ROOM_SETUP_STATUS};
    const uint8_t stage[29]={1,ROOM_SETUP_STAGE,1,8,'x','a','b','c','d','e','f','g','h'};
    const uint8_t save[26]={1,ROOM_SETUP_SAVE,0,0,0,0,0,0,0,1};
    room_phone_io io={&f,io_open,io_recv,io_send,no_legacy};
    reset();gate_init(&f.gate);expected_queue_owner=&t;assert(room_apsta_task_bind(&t));
    assert(room_apsta_task_open(&t,PHONE_PROFILE_MISSING,false,true,&store,ssid,1,key,16,&id,7));
    assert(room_phone_channel_open(&c,&t,io));
    current=&other_id;assert(!room_phone_channel_poll(&c));current=&owner_id;
    begin_packet(&f,&c,hs,sizeof hs,true,0);confirm_packet(&f,&c);
    for(unsigned i=0;i<8;i++){
        us+=101000;begin_packet(&f,&c,status,sizeof status,false,ROOM_REPLY_OK);confirm_packet(&f,&c);
    }
    us+=101000;begin_packet(&f,&c,stage,sizeof stage,false,ROOM_REPLY_OK);confirm_packet(&f,&c);
    assert(room_phone_channel_poll(&c)&&posted&&t.pretrial_phase&&!connects);
    deliver_barrier();assert(room_phone_channel_poll(&c)&&connects==1);
    linked=true;sta.up=true;sta.ip=1;
    ip_event_got_ip_t got={.esp_netif=&sta,.ip_info={.ip={.addr=1}}};
    handlers[0](args[0],IP_EVENT,IP_EVENT_STA_GOT_IP,&got);
    assert(room_phone_channel_poll(&c)&&t.session.bridge.phone.enrollment.state==ENROLL_VERIFIED);
    us+=101000;begin_packet(&f,&c,save,sizeof save,false,ROOM_REPLY_SAVED);
    assert(state.writes==1);confirm_packet(&f,&c);
    uint64_t deadline=(uint64_t)us/1000u+5000u;
    assert(!room_phone_channel_close_until(&c,deadline,gate_clock,NULL,timed_close));
    assert(!room_apsta_task_close_until(&t,false,deadline)&&!stops&&!destroys);
    f.exited=f.drained=true;assert(room_phone_channel_close_until(&c,deadline,gate_clock,NULL,timed_close));
    finish_radio(&t,deadline);
    /* Recovery uses the same persistent owner/gate, a newer window, and
     * independent stop proof. Cancel a borrowed handshake with busy gate. */
    uint64_t old=f.window;reset();us+=100000;
    assert(room_apsta_task_open(&t,PHONE_PROFILE_VALID,true,true,&store,ssid,1,key,16,&id,7));
    assert(room_phone_channel_open(&c,&t,io)&&f.window>old);
    begin_packet(&f,&c,hs,sizeof hs,true,0);
    deadline=(uint64_t)us/1000u+5000u;
    atomic_store(&f.gate.gate,GATE_BUSY);
    assert(!room_phone_channel_close_until(&c,deadline,gate_clock,NULL,timed_close));
    assert(atomic_load(&f.gate.gate)==(GATE_STOP|GATE_BUSY));
    mb_reply out;
    assert(ev(&f,(mb_event){.action=MB_SENT,.ticket=f.ticket,.flag=true},false,&out)==ACK_REJECT);
    assert(!room_apsta_task_close_until(&t,false,deadline)&&!stops&&t.session.transport.pending);
    atomic_fetch_and(&f.gate.gate,~(unsigned)GATE_BUSY);
    f.exited=true;assert(!room_phone_channel_close_until(&c,deadline,gate_clock,NULL,timed_close));
    assert(t.session.transport.pending&&c.reply&&!stops);
    f.drained=true;assert(room_phone_channel_close_until(&c,deadline,gate_clock,NULL,timed_close));
    assert(!t.session.transport.pending&&!c.reply);finish_radio(&t,deadline);
    assert(state.writes==1);
    puts("APSTA_GATE_CHANNEL_PASS exchanges=12 windows=2");return 0;
}
