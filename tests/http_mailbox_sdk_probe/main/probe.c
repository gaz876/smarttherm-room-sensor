#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "room_http_async.h"
#include "stop_gate_adapter.h"

/* Isolated QEMU probe. One synthetic request at a time, trusted local IDs.
 * No Wi-Fi driver, external interface, credentials or configuration writes.
 * One owner cleanup deadline covers mailbox and actual HTTP SDK stop;
 * only the observed SDK stop result authorizes mailbox reuse. */
#define CHECK(x) do { if(!(x)){printf("HTTP_MB_GATE_FAIL line=%d\n",__LINE__);fflush(stdout);abort();} } while(0)
typedef enum { NORMAL, APP_ERROR, CLOSE_PENDING, LATE } mode;
enum { REPEAT_ROUNDS=64 };
typedef struct {
    room_http_lifecycle life;
    room_http_async async[REPEAT_ROUNDS];
    unsigned round;
    room_lease transit;
    stop_gate_adapter box;
    SemaphoreHandle_t submitted, published, sent, allow_complete, completed;
    SemaphoreHandle_t allow_ack, responder_done, client_read, allow_client_close, client_done;
    uint64_t window, ticket;
    uint16_t port;
    mode test;
    bool claimed;
} fixture;
static fixture cases[4];
static room_sdk_stop worker;
static room_http_lifecycle forbidden;
static uint64_t now(void){int64_t us=esp_timer_get_time();CHECK(us>=0);return (uint64_t)us/1000u;}
static bool read_clock(void *ctx,uint64_t *ms){(void)ctx;*ms=now();return true;}
static void take(SemaphoreHandle_t s){CHECK(xSemaphoreTake(s,pdMS_TO_TICKS(4000))==pdTRUE);}
static void give(SemaphoreHandle_t s){CHECK(xSemaphoreGive(s)==pdTRUE);}
static void wipe(void *p,size_t n){volatile uint8_t *b=p;while(n--)*b++=0;}
static ack_result event(fixture *f,mb_event e,bool receipt,mb_reply *out){e.window=f->window;return gate_step(&f->box,e,receipt,out,read_clock,NULL);}
static ack_result poll_receipt(fixture *f,mb_reply *out){return event(f,(mb_event){.ticket=f->ticket},true,out);}
static ack_result simple(fixture *f,mb_action a,bool flag,bool drained){mb_reply out;return event(f,(mb_event){.action=a,.flag=flag,.drained=drained},false,&out);}
static const char *response(const fixture *f){return f->test==APP_ERROR?"PROBE_APP_ERROR":"PROBE_OK";}
static esp_err_t uri_handler(httpd_req_t *request){
    fixture *f=request->user_ctx;room_lease cb={0};mb_reply out;uint8_t body[3]={0};
    if(!room_http_enter(&f->life,f->window,&cb))return ESP_FAIL;
    if(f->claimed){CHECK(room_admission_leave(&cb));return ESP_FAIL;}
    f->claimed=true;CHECK(request->content_len==sizeof body);
    size_t used=0;while(used<sizeof body){int n=httpd_req_recv(request,(char *)body+used,sizeof body-used);CHECK(n>0&&(size_t)n<=sizeof body-used);used+=(size_t)n;}
    CHECK(room_http_enter(&f->life,f->window,&f->transit));
    CHECK(room_http_async_begin(&f->async[f->round],&f->life,f->window,&cb,request));
    CHECK(event(f,(mb_event){.action=MB_SUBMIT,.input=body,.size=sizeof body},false,&out)==ACK_OK);
    f->ticket=out.ticket;wipe(body,sizeof body);
    /* Server task outranks the owner/responder on this single core; no
     * original-request access after handoff. Contexts remain boot-lifetime. */
    SemaphoreHandle_t ready=f->submitted;CHECK(room_admission_leave(&cb));give(ready);return ESP_OK;
}
static void responder(void *ctx){
    fixture *f=ctx;mb_reply out;char body[32]={0};take(f->published);
    CHECK(event(f,(mb_event){.action=MB_TAKE_REPLY,.ticket=f->ticket,.output=(uint8_t *)body,.size=sizeof body},false,&out)==ACK_OK);
    CHECK(out.length==strlen(response(f))&&memcmp(body,response(f),out.length)==0);
    if(f->test==APP_ERROR)CHECK(httpd_resp_set_status(f->async[f->round].request,"503 Service Unavailable")==ESP_OK);
    CHECK(httpd_resp_send(f->async[f->round].request,body,(ssize_t)out.length)==ESP_OK);
    wipe(body,sizeof body);give(f->sent);take(f->allow_complete);
    CHECK(room_http_async_complete(&f->async[f->round]));CHECK(!room_http_async_complete(&f->async[f->round]));
    give(f->completed);take(f->allow_ack);
    bool closing=f->test==CLOSE_PENDING||f->test==LATE;
    CHECK(event(f,(mb_event){.action=MB_SENT,.ticket=f->ticket,.flag=true},false,&out)==(closing?ACK_REJECT:ACK_OK));
    CHECK(room_admission_leave(&f->transit));give(f->responder_done);vTaskDelete(NULL);
}
static void client(void *ctx){
    fixture *f=ctx;int fd=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);CHECK(fd>=0);
    struct timeval tv={.tv_sec=3,.tv_usec=0};
    CHECK(setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof tv)==0);
    CHECK(setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof tv)==0);
    struct sockaddr_in address={.sin_family=AF_INET,.sin_port=htons(f->port)};
    CHECK(inet_pton(AF_INET,"127.0.0.1",&address.sin_addr)==1);
    CHECK(connect(fd,(struct sockaddr *)&address,sizeof address)==0);
    char req[]="POST /probe HTTP/1.1\r\nHost: 127.0.0.1\r\nContent-Length: 3\r\n\r\nABC";
    req[sizeof(req)-3]=(char)('0'+f->round/10u);
    req[sizeof(req)-2]=(char)('0'+f->round%10u);
    size_t sent=0;while(sent<sizeof(req)-1){ssize_t n=send(fd,req+sent,sizeof(req)-1-sent,0);CHECK(n>0);sent+=(size_t)n;}
    char buf[512]={0};size_t used=0;
    while(!strstr(buf,response(f))){CHECK(used<sizeof(buf)-1);ssize_t n=recv(fd,buf+used,sizeof(buf)-1-used,0);CHECK(n>0);used+=(size_t)n;buf[used]='\0';}
    CHECK(strstr(buf,f->test==APP_ERROR?"HTTP/1.1 503":"HTTP/1.1 200")!=NULL);
    give(f->client_read);take(f->allow_client_close);CHECK(close(fd)==0);give(f->client_done);vTaskDelete(NULL);
}
static void scenario(fixture *f,unsigned w,mode m){
    f->window=w;f->port=(uint16_t)(18080u+2u*w);f->test=m;gate_init(&f->box);
    f->submitted=xSemaphoreCreateBinary();f->published=xSemaphoreCreateBinary();f->sent=xSemaphoreCreateBinary();
    f->allow_complete=xSemaphoreCreateBinary();f->completed=xSemaphoreCreateBinary();f->allow_ack=xSemaphoreCreateBinary();
    f->responder_done=xSemaphoreCreateBinary();f->client_read=xSemaphoreCreateBinary();
    f->allow_client_close=xSemaphoreCreateBinary();f->client_done=xSemaphoreCreateBinary();
    CHECK(f->submitted&&f->published&&f->sent&&f->allow_complete&&f->completed&&f->allow_ack&&f->responder_done&&f->client_read&&f->allow_client_close&&f->client_done);
    CHECK(room_http_prepare(&f->life,&worker,w));
    httpd_config_t config=HTTPD_DEFAULT_CONFIG();config.server_port=f->port;config.ctrl_port=(uint16_t)(f->port+1u);
    config.recv_wait_timeout=2;config.send_wait_timeout=2;
    httpd_handle_t server=NULL;CHECK(httpd_start(&server,&config)==ESP_OK);
    CHECK(room_http_adopt(&f->life,server,httpd_stop));
    CHECK(gate_open(&f->box,w,read_clock,NULL));
    room_lease owner_lease={0};CHECK(room_http_enter(&f->life,w,&owner_lease));
    httpd_uri_t uri={.uri="/probe",.method=HTTP_POST,.handler=uri_handler,.user_ctx=f};
    CHECK(httpd_register_uri_handler(server,&uri)==ESP_OK);
    unsigned rounds=m==NORMAL?REPEAT_ROUNDS:1u;
    size_t heap_warm=0,heap_min=SIZE_MAX,heap_end=0,largest_min=SIZE_MAX;
    unsigned task_warm=0,task_peak=0;
    uint64_t deadline=0;bool closing=m==CLOSE_PENDING||m==LATE;
    for(unsigned i=0;i<rounds;i++){
    f->round=i;f->claimed=false;
    CHECK(xTaskCreate(responder,"mb_response",4096,f,3,NULL)==pdPASS);
    CHECK(xTaskCreate(client,"mb_client",4096,f,3,NULL)==pdPASS);
    take(f->submitted);mb_reply out;uint8_t request[3]={0};char reply[32]={0};
    CHECK(event(f,(mb_event){.action=MB_TAKE_REQUEST,.ticket=f->ticket,.output=request,.size=sizeof request},false,&out)==ACK_OK);
    CHECK(out.length==3&&request[0]=='A'&&request[1]==(uint8_t)('0'+i/10u)&&request[2]==(uint8_t)('0'+i%10u));
    CHECK(f->ticket==(uint64_t)i+1u);wipe(request,sizeof request);
    CHECK(event(f,(mb_event){.action=MB_TAKE_REQUEST,.ticket=f->ticket,.output=request,.size=sizeof request},false,&out)==ACK_REJECT);
    size_t n=strlen(response(f));memcpy(reply,response(f),n);
    CHECK(event(f,(mb_event){.action=MB_PUBLISH,.ticket=f->ticket,.input=(const uint8_t *)reply,.size=n},false,&out)==ACK_OK);
    wipe(reply,sizeof reply);give(f->published);take(f->sent);take(f->client_read);
    CHECK(poll_receipt(f,&out)==ACK_WAIT&&out.ticket==0);
    uint8_t extra=1;CHECK(event(f,(mb_event){.action=MB_SUBMIT,.input=&extra,.size=1},false,&out)==ACK_REJECT);
    deadline=now()+(m==LATE?300u:(m==CLOSE_PENDING?4500u:1500u));
    if(closing){CHECK(gate_close_until(&f->box,w,deadline,false,false,read_clock,NULL)==ACK_WAIT);
        CHECK(f->box.inner.box.life.stop_deadline==deadline);CHECK(room_http_close(&f->life,w,deadline)==ROOM_SDK_WAIT);
        CHECK(!room_admission_drained(&f->life.gate,w));CHECK(simple(f,MB_DRAINED,true,true)==ACK_REJECT);
        if(m==LATE){while(now()<deadline)vTaskDelay(1);CHECK(room_http_close(&f->life,w,deadline)==ROOM_SDK_LOCKED);}}
    if(m==CLOSE_PENDING){
        while(now()+2200u<deadline)vTaskDelay(1);
        CHECK(gate_close_until(&f->box,w,deadline,false,false,read_clock,NULL)==ACK_WAIT);
        CHECK(room_http_close(&f->life,w,deadline)==ROOM_SDK_WAIT);
        CHECK(f->box.inner.box.life.stop_deadline==deadline);
    }
    give(f->allow_complete);take(f->completed);
    CHECK(poll_receipt(f,&out)==(closing?ACK_REJECT:ACK_WAIT));
    if(m==CLOSE_PENDING)CHECK(room_http_close(&f->life,w,deadline)==ROOM_SDK_WAIT);
    give(f->allow_ack);take(f->responder_done);
    if(!closing){CHECK(poll_receipt(f,&out)==ACK_OK&&out.ticket==f->ticket);CHECK(poll_receipt(f,&out)==ACK_REJECT);}
    else CHECK(poll_receipt(f,&out)==ACK_REJECT);
    give(f->allow_client_close);take(f->client_done);
    CHECK(f->async[i].state==ROOM_ASYNC_DONE&&!f->async[i].request&&!f->async[i].lease.gate&&!f->transit.gate);
    if(m==NORMAL){
        vTaskDelay(2);
        CHECK(heap_caps_check_integrity_all(true));
        size_t free_bytes=heap_caps_get_free_size(MALLOC_CAP_8BIT);
        unsigned tasks=(unsigned)uxTaskGetNumberOfTasks();
        size_t largest=heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
        if(i==3){heap_warm=free_bytes;task_warm=tasks;}
        if(i>=3){
            if(free_bytes<heap_min)heap_min=free_bytes;
            if(largest<largest_min)largest_min=largest;
            if(tasks>task_peak)task_peak=tasks;
            heap_end=free_bytes;
        }
        if((i+1u)%16u==0){printf("HTTP_MB_SOAK_SAMPLE round=%u free=%u largest=%u tasks=%u\n",i+1u,(unsigned)free_bytes,(unsigned)largest,tasks);fflush(stdout);}
    }
    }
    if(m==NORMAL){
        CHECK(heap_warm&&heap_min&&heap_min<=heap_warm&&heap_warm-heap_min<=16384u);
        CHECK(task_warm&&task_peak<=task_warm);
        printf("HTTP_MB_SOAK_PASS rounds=%u warm=%u min=%u end=%u largest_min=%u tasks_warm=%u tasks_peak=%u\n",rounds,(unsigned)heap_warm,(unsigned)heap_min,(unsigned)heap_end,(unsigned)largest_min,task_warm,task_peak);fflush(stdout);
        deadline=now()+1500u;
    }
    if(!closing){CHECK(gate_close_until(&f->box,w,deadline,false,false,read_clock,NULL)==ACK_WAIT);}
    CHECK(room_admission_leave(&owner_lease));
    room_sdk_stop_result rc=room_http_close(&f->life,w,deadline);
    while(rc==ROOM_SDK_WAIT){CHECK(now()<deadline);vTaskDelay(1);rc=room_http_close(&f->life,w,deadline);}
    bool locked=m==LATE;CHECK(rc==(locked?ROOM_SDK_LOCKED:ROOM_SDK_DRAINED));CHECK(worker.quarantined==locked);
    CHECK(gate_close_until(&f->box,w,deadline,!locked,!locked,read_clock,NULL)==(locked?ACK_REJECT:ACK_OK));
    CHECK(gate_open(&f->box,w+100,read_clock,NULL)==!locked);
    if(!locked)CHECK(gate_close_until(&f->box,w+100,now()+1000u,true,true,read_clock,NULL)==ACK_OK);
    if(locked)CHECK(!room_http_prepare(&forbidden,&worker,w+1));
    else {
        /* On this single-core fixture both higher-priority tasks have exited;
         * successful SDK stop also proves no callback retains the handles. */
        vTaskDelay(2);
        vSemaphoreDelete(f->submitted);vSemaphoreDelete(f->published);vSemaphoreDelete(f->sent);
        vSemaphoreDelete(f->allow_complete);vSemaphoreDelete(f->completed);vSemaphoreDelete(f->allow_ack);
        vSemaphoreDelete(f->responder_done);vSemaphoreDelete(f->client_read);
        vSemaphoreDelete(f->allow_client_close);vSemaphoreDelete(f->client_done);
        CHECK(heap_caps_check_integrity_all(true));
    }
    printf("HTTP_MB_GATE_CASE %u status=%u ack=%u result=%d\n",w,m==APP_ERROR?503u:200u,closing?0u:1u,(int)rc);fflush(stdout);
}
/* Deliberately blocked clock only in this isolated scheduling fixture.
 * Production clock callbacks must remain bounded and nonblocking. */
static stop_gate_adapter race_gate;
static SemaphoreHandle_t race_entered,race_release,race_done;
static ack_result race_result;
static mb_reply race_reply;
static bool race_clock(void *ctx,uint64_t *ms){(void)ctx;give(race_entered);take(race_release);return read_clock(NULL,ms);}
static void race_traffic(void *ctx){
    (void)ctx;uint8_t input[3]={1,2,3};
    race_result=gate_step(&race_gate,(mb_event){.action=MB_SUBMIT,.window=1,.input=input,.size=sizeof input},false,&race_reply,race_clock,NULL);
    give(race_done);vTaskDelete(NULL);
}
static void stop_race(void){
    gate_init(&race_gate);CHECK(gate_open(&race_gate,1,read_clock,NULL));
    race_entered=xSemaphoreCreateBinary();race_release=xSemaphoreCreateBinary();race_done=xSemaphoreCreateBinary();
    CHECK(race_entered&&race_release&&race_done);
    CHECK(xTaskCreate(race_traffic,"gate_race",4096,NULL,3,NULL)==pdPASS);take(race_entered);
    uint64_t deadline=now()+3000u;mb_reply out;uint8_t b=1;
    CHECK(gate_close_until(&race_gate,1,deadline,false,false,read_clock,NULL)==ACK_BUSY);
    CHECK(gate_step(&race_gate,(mb_event){.action=MB_SUBMIT,.window=1,.input=&b,.size=1},false,&out,read_clock,NULL)==ACK_REJECT);
    give(race_release);take(race_done);CHECK(race_result==ACK_REJECT&&!race_reply.ticket);
    CHECK(gate_close_until(&race_gate,1,deadline,true,false,read_clock,NULL)==ACK_WAIT);
    CHECK(gate_close_until(&race_gate,1,deadline,true,true,read_clock,NULL)==ACK_OK);
    CHECK(gate_open(&race_gate,2,read_clock,NULL));
    CHECK(gate_close_until(&race_gate,2,now()+1000u,true,true,read_clock,NULL)==ACK_OK);
    puts("HTTP_MB_GATE_RACE_PASS");fflush(stdout);
}
void app_main(void){
    CHECK(esp_netif_init()==ESP_OK);CHECK(esp_event_loop_create_default()==ESP_OK);CHECK(room_sdk_stop_bind(&worker));
    stop_race();
    for(unsigned i=0;i<4;i++)scenario(&cases[i],i+1,(mode)i);
    puts("HTTP_MB_GATE_PASS");fflush(stdout);
}
