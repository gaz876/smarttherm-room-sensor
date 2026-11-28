/* Standalone QEMU loopback fixture. No physical Wi-Fi, credentials or external interface.
 * Real SDK HTTP server, production driver/backend and stop worker. The only
 * test substitutions are AP name lookup and synthetic peer association. */
#include "room_http_idf_driver.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "freertos/semphr.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#define CHECK(x) do {if(!(x)){printf("SDK_BACKEND_FAIL line=%d\n",__LINE__);fflush(stdout);abort();}}while(0)
static room_http_backend backend;
static room_http_idf_driver driver;
static room_network_setup_backend port;
static SemaphoreHandle_t client_done;
static TaskHandle_t client_task;
static int ap_marker;
static atomic_bool cancel_round, handshake_round;
static atomic_uint serial;
static const uint8_t reply[]={'P','O','N','G'};
static esp_netif_t *fake_ap(void){return (esp_netif_t *)(void *)&ap_marker;}
static uint64_t now(void){int64_t us=esp_timer_get_time();CHECK(us>=0);return (uint64_t)us/1000u;}
/* Link wrap applies only to this isolated test. No changes to the real driver. */
esp_err_t __wrap_esp_netif_get_netif_impl_name(esp_netif_t *p,char *name){
    CHECK(p==fake_ap()&&name);
    memcpy(name,"lo0",4);
    return ESP_OK;
}
bool room_network_owner_prepare(unsigned key_id,const room_network_setup_backend *b,room_secure_status *s){
    (void)key_id;(void)b;(void)s;CHECK(false);return false;
}
static bool observe(void *ctx,const esp_netif_t *p,const httpd_req_t *req,room_http_peer_observation *o){
    CHECK(ctx==&ap_marker&&p==fake_ap()&&req&&o);
    int fd=httpd_req_to_sockfd((httpd_req_t *)req);
    struct sockaddr_in local={0},remote={0};socklen_t a=sizeof(local),b=sizeof(remote);
    if(fd<0||getsockname(fd,(struct sockaddr *)&local,&a)!=0||getpeername(fd,(struct sockaddr *)&remote,&b)!=0){
        printf("SDK_BACKEND_OBSERVE_FAIL socket errno=%d\n",errno);fflush(stdout);return false;
    }
    if(a!=sizeof(local)||b!=sizeof(remote)||local.sin_family!=AF_INET||remote.sin_family!=AF_INET||
       local.sin_addr.s_addr!=htonl(INADDR_LOOPBACK)||remote.sin_addr.s_addr!=htonl(INADDR_LOOPBACK)){
        printf("SDK_BACKEND_OBSERVE_FAIL family=%d/%d length=%u/%u\n",(int)local.sin_family,(int)remote.sin_family,(unsigned)a,(unsigned)b);
        fflush(stdout);return false;
    }
    /* Fixed test-local identity only. This does NOT qualify real AP evidence. */
    *o=(room_http_peer_observation){true,true,true,1,42};
    return true;
}
static void client(void *unused){
    (void)unused;
    for(;;){
        CHECK(ulTaskNotifyTake(pdTRUE,portMAX_DELAY)==1u);
        bool cancel=atomic_load(&cancel_round),hs=atomic_load(&handshake_round);
        unsigned seq=atomic_load(&serial);
        int fd=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);CHECK(fd>=0);
        struct timeval tv={.tv_sec=4,.tv_usec=0};
        CHECK(setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof tv)==0);
        CHECK(setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof tv)==0);
        struct sockaddr_in addr={.sin_family=AF_INET,.sin_port=htons(80)};
        CHECK(inet_pton(AF_INET,"127.0.0.1",&addr.sin_addr)==1);
        CHECK(connect(fd,(struct sockaddr *)&addr,sizeof addr)==0);
        char req[160];
        int len=snprintf(req,sizeof req,"POST %s HTTP/1.1\r\nHost: 127.0.0.1\r\nContent-Length: 3\r\nConnection: close\r\n\r\nA%02u",hs?"/setup/handshake":"/setup/packet",seq);
        CHECK(len>0&&(size_t)len<sizeof req&&seq<100u);
        size_t sent=0;
        while(sent<(size_t)len){ssize_t n=send(fd,req+sent,(size_t)len-sent,0);CHECK(n>0);sent+=(size_t)n;}
        char buf[512]={0};size_t used=0;
        if(cancel){
            ssize_t n=recv(fd,buf,sizeof buf-1,0);
            CHECK(n==0||(n<0&&(errno==ECONNRESET||errno==ECONNABORTED)));
        }else{
            while(!strstr(buf,"\r\n\r\nPONG")){
                CHECK(used<sizeof buf-1);
                ssize_t n=recv(fd,buf+used,sizeof buf-1-used,0);CHECK(n>0);
                used+=(size_t)n;buf[used]=0;
            }
            CHECK(strstr(buf,"HTTP/1.1 200")&&strstr(buf,"\r\n\r\nPONG"));
        }
        CHECK(close(fd)==0);
        CHECK(xSemaphoreGive(client_done)==pdTRUE);
    }
}
static void begin_round(unsigned seq,bool hs,bool cancel){
    atomic_store(&serial,seq);atomic_store(&handshake_round,hs);atomic_store(&cancel_round,cancel);
    xTaskNotifyGive(client_task);
    uint8_t in[8]={0};size_t n=0;bool observed=false;
    room_phone_io_result rc=ROOM_PHONE_IO_WAIT;uint64_t end=now()+2000u;
    while(rc==ROOM_PHONE_IO_WAIT){CHECK(now()<end);rc=port.io.receive(port.io.ctx,&observed,in,sizeof in,&n);if(rc==ROOM_PHONE_IO_WAIT)vTaskDelay(1);}
    CHECK(rc==ROOM_PHONE_IO_OK&&observed==hs&&n==3&&in[0]=='A'&&in[1]=='0'+seq/10u&&in[2]=='0'+seq%10u);
}
static void exchange(unsigned seq,bool hs){
    begin_round(seq,hs,false);
    size_t n=0;room_phone_io_result rc=ROOM_PHONE_IO_WAIT;uint64_t end=now()+2000u;
    CHECK(port.io.send(port.io.ctx,reply,sizeof reply,&n)==ROOM_PHONE_IO_WAIT&&n==0);
    while(rc==ROOM_PHONE_IO_WAIT){CHECK(now()<end);vTaskDelay(1);rc=port.io.send(port.io.ctx,reply,sizeof reply,&n);}
    CHECK(rc==ROOM_PHONE_IO_OK&&n==sizeof reply);
    CHECK(xSemaphoreTake(client_done,pdMS_TO_TICKS(4000))==pdTRUE);
    vTaskDelay(3);
}
static void finish_close(uint64_t w,uint64_t end){
    while(!port.close_until(port.io.ctx,w,end)){CHECK(!backend.failed&&now()<end);vTaskDelay(1);}
    CHECK(!backend.current&&!backend.failed&&port.close_until(port.io.ctx,w,end));
    for(size_t i=0;i<sizeof driver.scratch;++i)CHECK(!driver.scratch[i]);
    CHECK(heap_caps_check_integrity_all(true));
    vTaskDelay(3);
}
void app_main(void){
    CHECK(esp_netif_init()==ESP_OK);
    CHECK(esp_event_loop_create_default()==ESP_OK);
    atomic_init(&cancel_round,false);atomic_init(&handshake_round,false);atomic_init(&serial,0u);
    client_done=xSemaphoreCreateBinary();CHECK(client_done);
    CHECK(xTaskCreate(client,"loop_client",4096,NULL,3,&client_task)==pdPASS);
    room_http_driver d={0};
    CHECK(room_http_idf_driver_init(&driver,&ap_marker,observe,&d));
    CHECK(room_http_backend_prepare(&backend,d,&port));
    for(uint64_t w=1;w<=8;++w){
        CHECK(port.io.open(port.io.ctx,fake_ap(),w));
        exchange((unsigned)(2*w-2),true);
        exchange((unsigned)(2*w-1),false);
        CHECK(driver.guard.bound&&!driver.guard.failed);
        finish_close(w,now()+3000u);
    }
    puts("SDK_BACKEND_RESTART_PASS windows=8 exchanges=16");
    CHECK(port.io.open(port.io.ctx,fake_ap(),9));
    begin_round(16,true,true);
    uint64_t end=now()+3000u;
    CHECK(!port.close_until(port.io.ctx,9,end));
    finish_close(9,end);
    CHECK(xSemaphoreTake(client_done,pdMS_TO_TICKS(4000))==pdTRUE);
    puts("SDK_BACKEND_DRAIN_PASS");
    CHECK(port.io.open(port.io.ctx,fake_ap(),10));
    exchange(17,true);
    finish_close(10,now()+3000u);
    puts("SDK_BACKEND_RECOVERY_PASS");
    puts("SDK_BACKEND_PASS");fflush(stdout);
}
