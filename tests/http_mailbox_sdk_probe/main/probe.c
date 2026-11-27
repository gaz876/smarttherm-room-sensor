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
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "room_http_async.h"
#include "completion_adapter.h"

/* Isolated QEMU probe. One synthetic request at a time, trusted local IDs.
 * No Wi-Fi driver, external interface, credentials or configuration writes.
 * Independent legacy mailbox stop budget is not an owner deadline proof;
 * recovery always additionally requires successful actual HTTP SDK stop. */
#define CHECK(x) do { if(!(x)){printf("HTTP_MB_FAIL line=%d\n",__LINE__);fflush(stdout);abort();} } while(0)
typedef enum { NORMAL, APP_ERROR, CLOSE_PENDING, LATE } mode;
typedef struct {
    room_http_lifecycle life;
    room_http_async async;
    room_lease transit;
    ack_adapter box;
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
static ack_result event(fixture *f,mb_event e,bool receipt,mb_reply *out){e.window=f->window;return ack_step(&f->box,e,receipt,out,read_clock,NULL);}
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
    CHECK(room_http_async_begin(&f->async,&f->life,f->window,&cb,request));
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
    if(f->test==APP_ERROR)CHECK(httpd_resp_set_status(f->async.request,"503 Service Unavailable")==ESP_OK);
    CHECK(httpd_resp_send(f->async.request,body,(ssize_t)out.length)==ESP_OK);
    wipe(body,sizeof body);give(f->sent);take(f->allow_complete);
    CHECK(room_http_async_complete(&f->async));CHECK(!room_http_async_complete(&f->async));
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
    static const char req[]="POST /probe HTTP/1.1\r\nHost: 127.0.0.1\r\nContent-Length: 3\r\n\r\nABC";
    size_t sent=0;while(sent<sizeof(req)-1){ssize_t n=send(fd,req+sent,sizeof(req)-1-sent,0);CHECK(n>0);sent+=(size_t)n;}
    char buf[512]={0};size_t used=0;
    while(!strstr(buf,response(f))){CHECK(used<sizeof(buf)-1);ssize_t n=recv(fd,buf+used,sizeof(buf)-1-used,0);CHECK(n>0);used+=(size_t)n;buf[used]='\0';}
    CHECK(strstr(buf,f->test==APP_ERROR?"HTTP/1.1 503":"HTTP/1.1 200")!=NULL);
    give(f->client_read);take(f->allow_client_close);CHECK(close(fd)==0);give(f->client_done);vTaskDelete(NULL);
}
static void scenario(fixture *f,unsigned w,mode m){
    f->window=w;f->port=(uint16_t)(18080u+2u*w);f->test=m;ack_init(&f->box);
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
    CHECK(simple(f,MB_OPEN,false,false)==ACK_OK);CHECK(simple(f,MB_STARTED,true,false)==ACK_OK);
    room_lease owner_lease={0};CHECK(room_http_enter(&f->life,w,&owner_lease));
    httpd_uri_t uri={.uri="/probe",.method=HTTP_POST,.handler=uri_handler,.user_ctx=f};
    CHECK(httpd_register_uri_handler(server,&uri)==ESP_OK);
    CHECK(xTaskCreate(responder,"mb_response",4096,f,3,NULL)==pdPASS);
    CHECK(xTaskCreate(client,"mb_client",4096,f,3,NULL)==pdPASS);
    take(f->submitted);mb_reply out;uint8_t request[3]={0};char reply[32]={0};
    CHECK(event(f,(mb_event){.action=MB_TAKE_REQUEST,.ticket=f->ticket,.output=request,.size=sizeof request},false,&out)==ACK_OK);
    CHECK(out.length==3&&memcmp(request,"ABC",3)==0);wipe(request,sizeof request);
    CHECK(event(f,(mb_event){.action=MB_TAKE_REQUEST,.ticket=f->ticket,.output=request,.size=sizeof request},false,&out)==ACK_REJECT);
    size_t n=strlen(response(f));memcpy(reply,response(f),n);
    CHECK(event(f,(mb_event){.action=MB_PUBLISH,.ticket=f->ticket,.input=(const uint8_t *)reply,.size=n},false,&out)==ACK_OK);
    wipe(reply,sizeof reply);give(f->published);take(f->sent);take(f->client_read);
    CHECK(poll_receipt(f,&out)==ACK_WAIT&&out.ticket==0);
    uint8_t extra=1;CHECK(event(f,(mb_event){.action=MB_SUBMIT,.input=&extra,.size=1},false,&out)==ACK_REJECT);
    uint64_t deadline=now()+(m==LATE?300u:1500u);bool closing=m==CLOSE_PENDING||m==LATE;
    if(closing){CHECK(simple(f,MB_STOP,false,false)==ACK_OK);CHECK(room_http_close(&f->life,w,deadline)==ROOM_SDK_WAIT);
        CHECK(!room_admission_drained(&f->life.gate,w));CHECK(simple(f,MB_DRAINED,false,false)==ACK_REJECT);
        if(m==LATE){while(now()<deadline)vTaskDelay(1);CHECK(room_http_close(&f->life,w,deadline)==ROOM_SDK_LOCKED);}}
    give(f->allow_complete);take(f->completed);
    CHECK(poll_receipt(f,&out)==(closing?ACK_REJECT:ACK_WAIT));
    if(m==CLOSE_PENDING)CHECK(room_http_close(&f->life,w,deadline)==ROOM_SDK_WAIT);
    give(f->allow_ack);take(f->responder_done);
    if(!closing){CHECK(poll_receipt(f,&out)==ACK_OK&&out.ticket==f->ticket);CHECK(poll_receipt(f,&out)==ACK_REJECT);}
    else CHECK(poll_receipt(f,&out)==ACK_REJECT);
    give(f->allow_client_close);take(f->client_done);
    CHECK(simple(f,MB_STOP,false,false)==ACK_OK);CHECK(room_admission_leave(&owner_lease));
    room_sdk_stop_result rc=room_http_close(&f->life,w,deadline);
    while(rc==ROOM_SDK_WAIT){CHECK(now()<deadline);vTaskDelay(1);rc=room_http_close(&f->life,w,deadline);}
    bool locked=m==LATE;CHECK(rc==(locked?ROOM_SDK_LOCKED:ROOM_SDK_DRAINED));CHECK(worker.quarantined==locked);
    CHECK(simple(f,MB_DRAINED,!locked,!locked)==(locked?ACK_REJECT:ACK_OK));
    CHECK(ack_step(&f->box,(mb_event){.action=MB_OPEN,.window=w+100},false,&out,read_clock,NULL)==(locked?ACK_REJECT:ACK_OK));
    if(locked)CHECK(!room_http_prepare(&forbidden,&worker,w+1));
    printf("HTTP_MB_CASE %u status=%u ack=%u result=%d\n",w,m==APP_ERROR?503u:200u,closing?0u:1u,(int)rc);fflush(stdout);
}
void app_main(void){
    CHECK(esp_netif_init()==ESP_OK);CHECK(esp_event_loop_create_default()==ESP_OK);CHECK(room_sdk_stop_bind(&worker));
    for(unsigned i=0;i<4;i++)scenario(&cases[i],i+1,(mode)i);
    puts("HTTP_MB_PASS");fflush(stdout);
}
