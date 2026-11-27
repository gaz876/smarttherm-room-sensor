#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include "esp_netif.h"
#include "esp_timer.h"
#include "freertos/semphr.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "room_http_async.h"

/* Standalone test firmware only. No Wi-Fi, network driver, real device,
 * credentials or external peer. The runner also disables QEMU's NIC. */
#define CHECK(x) do { if(!(x)){printf("HTTP_REAL_FAIL line=%d\n",__LINE__);fflush(stdout);abort();} } while(0)
typedef struct {
    room_http_lifecycle life;
    room_http_async async;
    SemaphoreHandle_t request_ready, response_sent, allow_complete;
    SemaphoreHandle_t complete_done, client_read, allow_client_close, client_done;
    uint64_t window;
    uint16_t port;
    bool app_error;
    bool claimed; /* Only the server task touches this field. */
} fixture;
static fixture cases[3];
static room_sdk_stop worker;
static uint64_t now_ms(void){int64_t us=esp_timer_get_time();CHECK(us>=0);return (uint64_t)us/1000u;}
static void take(SemaphoreHandle_t s){CHECK(xSemaphoreTake(s,pdMS_TO_TICKS(4000))==pdTRUE);}
static void give(SemaphoreHandle_t s){CHECK(xSemaphoreGive(s)==pdTRUE);}
static esp_err_t uri_handler(httpd_req_t *request){
    fixture *f=request->user_ctx;room_lease callback={0};
    if(!room_http_enter(&f->life,f->window,&callback))return ESP_FAIL;
    if(f->claimed){CHECK(room_admission_leave(&callback));return ESP_FAIL;}
    f->claimed=true;
    CHECK(room_http_async_begin(&f->async,&f->life,f->window,&callback,request));
    /* One immutable boot-lifetime context, one request. The signal is the
     * handoff; the handler has no further access to either request copy.
     * The server priority is above the responder's on this single core. */
    SemaphoreHandle_t ready=f->request_ready;
    CHECK(room_admission_leave(&callback));give(ready);return ESP_OK;
}
static void responder(void *ctx){
    fixture *f=ctx;take(f->request_ready);
    CHECK(f->async.request!=NULL);
    if(f->app_error)CHECK(httpd_resp_set_status(f->async.request,"503 Service Unavailable")==ESP_OK);
    const char *body=f->app_error?"PROBE_APP_ERROR":"PROBE_OK";
    CHECK(httpd_resp_send(f->async.request,body,HTTPD_RESP_USE_STRLEN)==ESP_OK);
    give(f->response_sent);take(f->allow_complete);
    CHECK(room_http_async_complete(&f->async));CHECK(!room_http_async_complete(&f->async));
    give(f->complete_done);vTaskDelete(NULL);
}
static void client(void *ctx){
    fixture *f=ctx;int fd=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);CHECK(fd>=0);
    struct timeval tv={.tv_sec=3,.tv_usec=0};
    CHECK(setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof tv)==0);
    CHECK(setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof tv)==0);
    struct sockaddr_in address={.sin_family=AF_INET,.sin_port=htons(f->port)};
    CHECK(inet_pton(AF_INET,"127.0.0.1",&address.sin_addr)==1);
    CHECK(connect(fd,(struct sockaddr *)&address,sizeof address)==0);
    static const char request[]="GET /probe HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n";
    size_t sent=0;
    while(sent<sizeof(request)-1){ssize_t n=send(fd,request+sent,sizeof(request)-1-sent,0);CHECK(n>0);sent+=(size_t)n;}
    char response[512]={0};size_t used=0;
    const char *body=f->app_error?"PROBE_APP_ERROR":"PROBE_OK";
    while(!strstr(response,body)){
        CHECK(used<sizeof(response)-1);ssize_t n=recv(fd,response+used,sizeof(response)-1-used,0);
        CHECK(n>0);used+=(size_t)n;response[used]='\0';
    }
    CHECK(strstr(response,f->app_error?"HTTP/1.1 503":"HTTP/1.1 200")!=NULL);
    give(f->client_read);take(f->allow_client_close);CHECK(close(fd)==0);
    give(f->client_done);vTaskDelete(NULL);
}
static void scenario(fixture *f,uint64_t w,bool app_error,bool late){
    f->window=w;f->port=(uint16_t)(18080u+2u*w);f->app_error=app_error;
    f->request_ready=xSemaphoreCreateBinary();f->response_sent=xSemaphoreCreateBinary();
    f->allow_complete=xSemaphoreCreateBinary();f->complete_done=xSemaphoreCreateBinary();
    f->client_read=xSemaphoreCreateBinary();f->allow_client_close=xSemaphoreCreateBinary();f->client_done=xSemaphoreCreateBinary();
    CHECK(f->request_ready&&f->response_sent&&f->allow_complete&&f->complete_done&&f->client_read&&f->allow_client_close&&f->client_done);
    CHECK(room_http_prepare(&f->life,&worker,w));
    httpd_config_t config=HTTPD_DEFAULT_CONFIG();config.server_port=f->port;
    config.ctrl_port=(uint16_t)(f->port+1u);config.recv_wait_timeout=2;config.send_wait_timeout=2;
    httpd_handle_t server=NULL;CHECK(httpd_start(&server,&config)==ESP_OK);
    CHECK(room_http_adopt(&f->life,server,httpd_stop));
    httpd_uri_t uri={.uri="/probe",.method=HTTP_GET,.handler=uri_handler,.user_ctx=f};
    CHECK(httpd_register_uri_handler(server,&uri)==ESP_OK);
    CHECK(xTaskCreate(responder,"probe_response",4096,f,3,NULL)==pdPASS);
    CHECK(xTaskCreate(client,"probe_client",4096,f,3,NULL)==pdPASS);
    take(f->response_sent);take(f->client_read);
    uint64_t deadline=now_ms()+(late?300u:5000u);
    CHECK(room_http_close(&f->life,w,deadline)==ROOM_SDK_WAIT);
    CHECK(f->life.handle==server&&worker.reservation==&f->life);
    CHECK(!room_admission_drained(&f->life.gate,w));
    if(late){while(now_ms()<deadline)vTaskDelay(1);CHECK(room_http_close(&f->life,w,deadline)==ROOM_SDK_LOCKED);}
    give(f->allow_complete);take(f->complete_done);
    give(f->allow_client_close);take(f->client_done);
    CHECK(room_admission_drained(&f->life.gate,w));
    room_sdk_stop_result rc=room_http_close(&f->life,w,deadline);
    while(rc==ROOM_SDK_WAIT){CHECK(now_ms()<deadline);vTaskDelay(1);rc=room_http_close(&f->life,w,deadline);}
    CHECK(rc==(late?ROOM_SDK_LOCKED:ROOM_SDK_DRAINED));
    CHECK(worker.quarantined==late);
    CHECK(late?(worker.reservation==&f->life):(worker.reservation==NULL&&f->life.handle==NULL));
    printf("HTTP_REAL_CASE %u status=%u result=%d\n",(unsigned)w,app_error?503u:200u,(int)rc);fflush(stdout);
    /* All contexts stay alive until test boot ends. Late case deliberately
     * retains its quarantined server, with no retry or forced cleanup. */
}
void app_main(void){
    CHECK(esp_netif_init()==ESP_OK);CHECK(room_sdk_stop_bind(&worker));
    scenario(&cases[0],1,false,false);scenario(&cases[1],2,true,false);scenario(&cases[2],3,false,true);
    puts("HTTP_REAL_PASS");fflush(stdout);
}
