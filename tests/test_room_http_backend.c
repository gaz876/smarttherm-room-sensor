#include "room_http_idf_driver.h"
#include "room_http_idf_peer.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "lwip/sockets.h"
#include <net/if.h>
#include "room_wifi_identity_provider.h"
#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
static _Thread_local int thread_id;
static pthread_t stop_thread;
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cv=PTHREAD_COND_INITIALIZER;
static bool created,quit,notified;
static atomic_int stops;
static atomic_int_fast64_t us;
static void (*worker_fn)(void *);static void *worker_arg;
static int worker_marker,handle,good_request,bad_request;
static bool fail_stop,fail_publish,fail_worker;
static room_http_start_result start_result=ROOM_HTTP_START_READY;
static unsigned starts,publishes,binds;
static room_http_backend_window *published;
static room_network_setup_backend attached;
struct esp_netif {int kind;};static struct esp_netif ap={1},other={2};
TaskHandle_t xTaskGetCurrentTaskHandle(void){return &thread_id;}
int64_t esp_timer_get_time(void){return atomic_load(&us);}
static void *worker_entry(void *unused){(void)unused;worker_fn(worker_arg);return NULL;}
int xTaskCreate(void (*fn)(void *),const char *name,unsigned stack,void *arg,unsigned priority,TaskHandle_t *out){
    assert(!created&&fn&&name&&stack&&priority&&out);
    if(fail_worker)return 0;
    worker_fn=fn;worker_arg=arg;*out=&worker_marker;
    assert(!pthread_create(&stop_thread,NULL,worker_entry,NULL));created=true;return pdPASS;
}
uint32_t ulTaskNotifyTake(int clear,uint32_t ticks){
    assert(clear==pdTRUE&&ticks==portMAX_DELAY);assert(!pthread_mutex_lock(&lock));
    while(!notified&&!quit)assert(!pthread_cond_wait(&cv,&lock));
    if(quit){assert(!pthread_mutex_unlock(&lock));pthread_exit(NULL);}
    notified=false;assert(!pthread_mutex_unlock(&lock));return 1;
}
void xTaskNotifyGive(TaskHandle_t t){assert(t==&worker_marker);assert(!pthread_mutex_lock(&lock));notified=true;assert(!pthread_cond_signal(&cv));assert(!pthread_mutex_unlock(&lock));}
static void join_worker(void){if(!created)return;assert(!pthread_mutex_lock(&lock));quit=true;assert(!pthread_cond_signal(&cv));assert(!pthread_mutex_unlock(&lock));assert(!pthread_join(stop_thread,NULL));}
static room_http_start_result start(void *ctx,const esp_netif_t *p,httpd_handle_t *h){assert(ctx==&ap&&p==&ap);++starts;*h=start_result==ROOM_HTTP_START_CLEAN_FAILURE?NULL:&handle;return start_result;}
static bool publish(void *ctx,httpd_handle_t h,room_http_backend_window *w){assert(ctx==&ap&&h==&handle&&w&&w->life.phase==HTTP_LIFE_OPEN);++publishes;published=w;return !fail_publish;}
static esp_err_t stop(httpd_handle_t h){assert(h==&handle);atomic_fetch_add(&stops,1);return fail_stop?1:0;}
static bool peer(void *ctx,const esp_netif_t *p,const void *req){return ctx==&ap&&p==&ap&&req==&good_request;}
static room_http_driver driver(void){return (room_http_driver){&ap,start,publish,stop,peer};}
bool room_secure_init(unsigned n){return n==2;}
room_secure_status room_secure_load(room_enroll_profile *p){*p=(room_enroll_profile){0};return ROOM_SEC_MISSING;}
void enroll_wipe(void *v,size_t n){memset(v,0,n);}
bool room_wifi_idf_init(const uint8_t *a,size_t n,const uint8_t *b,size_t m){(void)a;(void)n;(void)b;(void)m;assert(0);return false;}
bool room_wifi_apsta_link_bind(room_wifi_apsta_provider p){(void)p;assert(0);return false;}
bool room_wifi_apsta_link_init_missing(room_wifi_apsta_provider p){(void)p;++binds;return true;}
bool room_wifi_identity_provider_init(room_wifi_apsta_provider *p){*p=(room_wifi_apsta_provider){0};return true;}
bool room_wifi_apsta_link_attach_io_until(room_phone_io io,room_phone_timed_close close){attached=(room_network_setup_backend){io,close};return true;}
static room_http_backend backend;
static const uint8_t req[]={1,2,3},reply[]={4,5,6,7};
static void finish_close(uint64_t window,uint64_t deadline){
    for(unsigned i=0;i<1000000;++i){if(attached.close_until(attached.io.ctx,window,deadline))return;assert(!backend.failed);sched_yield();}
    assert(0);
}
static void setup(void){room_secure_status s;assert(room_http_backend_prepare_owner(&backend,driver(),2,&s)&&s==ROOM_SEC_MISSING&&binds==1);assert(attached.io.ctx==&backend);}
static void exchange(void){
    room_http_exchange x={0},busy={0};uint8_t buf[4096];size_t n=999;bool hs=false;
    assert(!room_http_backend_offer(published,&x,&bad_request,true,req,sizeof(req)));
    assert(!room_http_backend_offer(published,&x,&good_request,true,req,4097));
    assert(attached.io.receive(attached.io.ctx,&hs,buf,sizeof(buf),&n)==ROOM_PHONE_IO_WAIT&&n==0);
    assert(room_http_backend_offer(published,&x,&good_request,true,req,sizeof(req)));
    assert(!room_http_backend_offer(published,&busy,&good_request,false,req,sizeof(req)));
    assert(room_http_backend_reply(&x,buf,sizeof(buf),&n)==ROOM_PHONE_IO_WAIT&&n==0);
    assert(attached.io.receive(attached.io.ctx,&hs,buf,sizeof(buf),&n)==ROOM_PHONE_IO_OK&&hs&&n==sizeof(req)&&!memcmp(buf,req,n));
    assert(attached.io.send(attached.io.ctx,reply,sizeof(reply),&n)==ROOM_PHONE_IO_WAIT&&n==0);
    assert(room_http_backend_reply(&x,buf,sizeof(buf),&n)==ROOM_PHONE_IO_OK&&n==sizeof(reply)&&!memcmp(buf,reply,n));
    assert(attached.io.send(attached.io.ctx,reply,sizeof(reply),&n)==ROOM_PHONE_IO_WAIT&&n==0);
    assert(room_http_backend_finish(&x,true));assert(!room_http_backend_finish(&x,true));
    assert(attached.io.send(attached.io.ctx,reply,sizeof(reply),&n)==ROOM_PHONE_IO_OK&&n==sizeof(reply));
    assert(attached.io.receive(attached.io.ctx,&hs,buf,sizeof(buf),&n)==ROOM_PHONE_IO_WAIT);
}
static void scenario(const char *mode){
    if(!strcmp(mode,"worker")){fail_worker=true;room_secure_status s;assert(!room_http_backend_prepare_owner(&backend,driver(),2,&s)&&backend.failed&&!binds);return;}
    setup();
    if(!strcmp(mode,"clean")){start_result=ROOM_HTTP_START_CLEAN_FAILURE;assert(!attached.io.open(attached.io.ctx,&ap,1));assert(attached.close_until(attached.io.ctx,1,5000));assert(!atomic_load(&stops));start_result=ROOM_HTTP_START_READY;assert(attached.io.open(attached.io.ctx,&ap,2));finish_close(2,5000);return;}
    if(!strcmp(mode,"uncertain")){start_result=ROOM_HTTP_START_UNCERTAIN;assert(!attached.io.open(attached.io.ctx,&ap,1)&&backend.failed);assert(!attached.close_until(attached.io.ctx,1,5000));assert(!publishes&&!atomic_load(&stops));assert(!attached.io.open(attached.io.ctx,&ap,2));return;}
    if(!strcmp(mode,"publish")){fail_publish=true;assert(!attached.io.open(attached.io.ctx,&ap,1));finish_close(1,5000);assert(atomic_load(&stops)==1);fail_publish=false;assert(attached.io.open(attached.io.ctx,&ap,2));finish_close(2,5000);assert(atomic_load(&stops)==2);return;}
    assert(attached.io.open(attached.io.ctx,&ap,1));
    if(!strcmp(mode,"normal")){
        for(unsigned i=0;i<128;++i){exchange();}
        finish_close(1,5000);assert(atomic_load(&stops)==1);assert(!attached.close_until(attached.io.ctx,1,4999));
        /* SDK stop must retire callbacks; no old window pointer is used after it. */
        for(uint64_t window=2;window<=128;++window){
            assert(!attached.io.open(attached.io.ctx,&ap,window-1));
            assert(attached.io.open(attached.io.ctx,&ap,window));
            assert(!attached.io.open(attached.io.ctx,&ap,window+1));
            assert(published==&backend.storage&&published->life.window==window);
            exchange();finish_close(window,5000);
            assert(atomic_load(&stops)==(int)window&&!backend.current);
        }
        assert(!attached.io.open(attached.io.ctx,NULL,129));
        assert(!attached.io.open(attached.io.ctx,&ap,130));
        assert(attached.close_until(attached.io.ctx,129,5000));
        assert(!attached.close_until(attached.io.ctx,129,4999));
        assert(!attached.io.open(attached.io.ctx,&ap,129));
        assert(attached.io.open(attached.io.ctx,&ap,130));finish_close(130,5000);
        assert(atomic_load(&stops)==129);printf("BACKEND_RESTART_WINDOWS=%d\n",atomic_load(&stops));return;
    }
    if(!strcmp(mode,"abort")||!strcmp(mode,"drain")){
        room_http_exchange x={0};assert(room_http_backend_offer(published,&x,&good_request,true,req,sizeof(req)));
        if(!strcmp(mode,"drain")){assert(!attached.close_until(attached.io.ctx,1,5000));assert(!atomic_load(&stops));assert(!attached.io.open(attached.io.ctx,&ap,2));}
        assert(!room_http_backend_finish(&x,false));uint8_t b[8];size_t n;bool hs;
        assert(attached.io.receive(attached.io.ctx,&hs,b,sizeof(b),&n)==ROOM_PHONE_IO_ERROR);finish_close(1,5000);assert(atomic_load(&stops)==1);return;
    }
    if(!strcmp(mode,"deadline")){room_http_exchange x={0};assert(room_http_backend_offer(published,&x,&good_request,true,req,sizeof(req)));assert(!attached.close_until(attached.io.ctx,1,5000));atomic_store(&us,5000000);assert(!attached.close_until(attached.io.ctx,1,5000)&&backend.failed);assert(!room_http_backend_finish(&x,false));assert(!atomic_load(&stops));assert(!attached.io.open(attached.io.ctx,&ap,2));return;}
    if(!strcmp(mode,"stop-error")){fail_stop=true;for(unsigned i=0;i<1000000&&!backend.failed;++i){assert(!attached.close_until(attached.io.ctx,1,5000));sched_yield();}assert(backend.failed&&atomic_load(&stops)==1);assert(!attached.io.open(attached.io.ctx,&ap,2));return;}
    assert(0);
}
static httpd_uri_t routes[2];
static unsigned routes_count;
static bool idf_name_error,idf_start_error,idf_register_error,idf_peer_error,idf_receive_error,idf_send_error,idf_timeout;
static atomic_uint body_reads,body_sent;
static uint8_t received_reply[16];
static size_t read_offset;
static httpd_req_t sdk_request;
static esp_err_t handler_result;
static room_http_idf_driver sdk_driver;
static uint64_t association=1,identity=42;
/* Synthetic read-only SDK evidence; no actual sockets or radio. */
static unsigned evidence_fault,evidence_epoch_reads,evidence_list_reads;
static const uint8_t peer_mac[]={2,0,0,0,0,42};
static const uint32_t ap_ip=UINT32_C(0x0100000a),peer_ip=UINT32_C(0x0200000a);
bool esp_netif_is_netif_up(esp_netif_t *p){return p==&ap&&evidence_fault!=1;}
esp_err_t esp_netif_get_ip_info(esp_netif_t *p,esp_netif_ip_info_t *i){assert(p==&ap);i->ip.addr=ap_ip;return evidence_fault==2?ESP_FAIL:ESP_OK;}
esp_err_t esp_netif_dhcps_get_status(esp_netif_t *p,esp_netif_dhcp_status_t *st){assert(p==&ap);*st=evidence_fault==3?ESP_NETIF_DHCP_INIT:ESP_NETIF_DHCP_STARTED;return ESP_OK;}
esp_err_t esp_wifi_ap_get_sta_list(wifi_sta_list_t *v){++evidence_list_reads;v->num=evidence_fault==4?0:evidence_fault==5?2:1;memcpy(v->sta[0].mac,peer_mac,6);if(evidence_fault==13&&evidence_list_reads%2==0)v->sta[0].mac[5]++;return evidence_fault==6?ESP_FAIL:ESP_OK;}
esp_err_t esp_netif_dhcps_get_clients_by_mac(esp_netif_t *p,int n,esp_netif_pair_mac_ip_t *v){assert(p==&ap&&n==1&&!memcmp(v->mac,peer_mac,6));v->ip.addr=evidence_fault==7?0:peer_ip;return evidence_fault==8?ESP_FAIL:ESP_OK;}
int httpd_req_to_sockfd(httpd_req_t *r){assert(r==&sdk_request);return evidence_fault==9?-1:123;}
static int mock_endpoint(int fd,struct sockaddr *s,socklen_t *len,bool local){assert(fd==123&&*len==sizeof(struct sockaddr_in));struct sockaddr_in addr={0};addr.sin_family=evidence_fault==12?AF_INET6:AF_INET;addr.sin_addr.s_addr=local?(evidence_fault==10?peer_ip:ap_ip):(evidence_fault==11?ap_ip:peer_ip);memcpy(s,&addr,sizeof(addr));return evidence_fault==14?-1:0;}
int mock_getsockname(int fd,struct sockaddr *s,socklen_t *n){return mock_endpoint(fd,s,n,true);}
int mock_getpeername(int fd,struct sockaddr *s,socklen_t *n){return mock_endpoint(fd,s,n,false);}
static bool local_epoch(void *ctx,const esp_netif_t *p,const uint8_t mac[6],uint64_t *token){assert(ctx==&ap&&p==&ap&&!memcmp(mac,peer_mac,6));++evidence_epoch_reads;*token=association+(evidence_fault==15&&evidence_epoch_reads%2==0?1u:0u);return evidence_fault!=16;}
static room_http_idf_peer peer_source={&ap,local_epoch};
static void evidence_case(void){
    room_http_peer_observation o={0};
    assert(room_http_idf_peer_observe(&peer_source,&ap,&sdk_request,&o));
    assert(o.association==association&&o.peer&&o.current_ap&&o.single_peer&&o.endpoint_matches);
    for(evidence_fault=1;evidence_fault<=16;++evidence_fault){
        evidence_epoch_reads=evidence_list_reads=0;
        assert(!room_http_idf_peer_observe(&peer_source,&ap,&sdk_request,&o));
        assert(!o.current_ap&&!o.single_peer&&!o.endpoint_matches&&!o.association&&!o.peer);
    }
    evidence_fault=0;assert(!room_http_idf_peer_observe(&peer_source,&other,&sdk_request,&o));
    room_http_idf_peer no_epoch={0};assert(!room_http_idf_peer_observe(&no_epoch,&ap,&sdk_request,&o));
    printf("SDK_EVIDENCE_REJECTIONS=18\n");
}
static bool trusted_peer(void *ctx,const esp_netif_t *p,const httpd_req_t *r,room_http_peer_observation *o){
    if(ctx!=&ap||p!=&ap||r!=&sdk_request||idf_peer_error)return false;
    *o=(room_http_peer_observation){true,true,true,association,identity};return true;
}
esp_err_t esp_netif_get_netif_impl_name(esp_netif_t *p,char *name){assert(p==&ap);if(idf_name_error)return ESP_FAIL;memcpy(name,"ap0",4);return ESP_OK;}
esp_err_t httpd_start(httpd_handle_t *h,const httpd_config_t *c){
    assert(c->if_name&&!strcmp(c->if_name->ifr_name,"ap0"));
    assert(c->max_open_sockets==1&&c->max_uri_handlers==2&&c->recv_wait_timeout==1&&c->send_wait_timeout==1&&!c->lru_purge_enable);
    ++starts;if(idf_start_error)return ESP_FAIL;*h=&handle;return ESP_OK;
}
esp_err_t httpd_stop(httpd_handle_t h){esp_err_t rc=stop(h);if(rc==ESP_OK){memset(routes,0,sizeof(routes));routes_count=0;}return rc;}
esp_err_t httpd_register_uri_handler(httpd_handle_t h,const httpd_uri_t *u){
    assert(h==&handle&&u->method==HTTP_POST&&u->handler&&u->user_ctx&&routes_count<2);
    if(idf_register_error&&routes_count==1)return ESP_FAIL;
    routes[routes_count++]=*u;return ESP_OK;
}
int httpd_req_recv(httpd_req_t *r,char *out,size_t cap){
    assert(r==&sdk_request&&cap&&read_offset<sizeof(req));atomic_fetch_add(&body_reads,1);
    if(idf_receive_error)return -1;
    *out=(char)req[read_offset++];return 1;
}
esp_err_t httpd_resp_set_type(httpd_req_t *r,const char *s){assert(r==&sdk_request&&!strcmp(s,"application/octet-stream"));return ESP_OK;}
esp_err_t httpd_resp_set_hdr(httpd_req_t *r,const char *k,const char *v){assert(r==&sdk_request&&!strcmp(k,"Cache-Control")&&!strcmp(v,"no-store"));return ESP_OK;}
esp_err_t httpd_resp_send(httpd_req_t *r,const char *p,size_t n){assert(r==&sdk_request&&n==sizeof(reply));if(idf_send_error)return ESP_FAIL;memcpy(received_reply,p,n);atomic_fetch_add(&body_sent,1);return ESP_OK;}
void vTaskDelay(TickType_t t){assert(t);if(idf_timeout)atomic_fetch_add(&us,10000);sched_yield();}
static void *handler_entry(void *unused){(void)unused;handler_result=routes[0].handler(&sdk_request);return NULL;}
static void sdk_round(void){
    sdk_request=(httpd_req_t){.user_ctx=routes[0].user_ctx,.content_len=sizeof(req),.handle=&handle};
    read_offset=0;pthread_t th;assert(!pthread_create(&th,NULL,handler_entry,NULL));
    uint8_t b[8];size_t n=0;bool hs=false;room_phone_io_result rc=ROOM_PHONE_IO_WAIT;
    for(unsigned i=0;i<1000000&&rc==ROOM_PHONE_IO_WAIT;++i){rc=attached.io.receive(attached.io.ctx,&hs,b,sizeof(b),&n);sched_yield();}
    assert(rc==ROOM_PHONE_IO_OK&&hs&&n==sizeof(req)&&!memcmp(b,req,n));rc=ROOM_PHONE_IO_WAIT;
    for(unsigned i=0;i<1000000&&rc==ROOM_PHONE_IO_WAIT;++i){rc=attached.io.send(attached.io.ctx,reply,sizeof(reply),&n);sched_yield();}
    assert(!pthread_join(th,NULL)&&handler_result==ESP_OK&&rc==ROOM_PHONE_IO_OK&&n==sizeof(reply));
}
static void sdk_restart(void){
    room_http_driver d={0};room_secure_status s;
    assert(room_http_idf_driver_init(&sdk_driver,&peer_source,room_http_idf_peer_observe,&d));
    assert(room_http_backend_prepare_owner(&backend,d,2,&s));
    for(uint64_t w=1;w<=24;++w){
        assert(attached.io.open(attached.io.ctx,&ap,w)&&routes_count==2&&!sdk_driver.guard.bound);
        sdk_round();assert(sdk_driver.guard.bound&&!sdk_driver.guard.failed);
        sdk_round();
        unsigned before=atomic_load(&body_reads);++association;
        assert(routes[0].handler(&sdk_request)==ESP_FAIL&&sdk_driver.guard.failed);
        assert(atomic_load(&body_reads)==before);
        --association;assert(routes[0].handler(&sdk_request)==ESP_FAIL&&atomic_load(&body_reads)==before);
        uint8_t b[8];size_t n;bool hs;assert(attached.io.receive(attached.io.ctx,&hs,b,sizeof(b),&n)==ROOM_PHONE_IO_ERROR);
        finish_close(w,5000);assert(routes_count==0&&atomic_load(&stops)==(int)w);
        ++association;++identity;
    }
    printf("SDK_RESTART_WINDOWS=24 successful_exchanges=%u\n",atomic_load(&body_sent));
}
static void peer_guard_case(void){
    room_http_peer_guard g={0};room_http_peer_observation o={true,true,true,1,42};
    assert(!room_http_peer_guard_check(&g,false,&o)&&!g.bound&&!g.failed);
    for(unsigned i=0;i<7;++i){
        room_http_peer_observation bad=o;
        switch(i){case 0:bad.current_ap=false;break;case 1:bad.single_peer=false;break;case 2:bad.endpoint_matches=false;break;case 3:bad.association=0;break;case 4:bad.peer=0;break;case 5:bad.association++;break;case 6:bad.peer++;break;}
        g=(room_http_peer_guard){0};assert(room_http_peer_guard_check(&g,true,&o));
        assert(room_http_peer_guard_check(&g,true,&o));
        assert(!room_http_peer_guard_check(&g,true,&bad)&&g.failed);
        assert(!room_http_peer_guard_check(&g,true,&o));
    }
    g=(room_http_peer_guard){0};assert(room_http_peer_guard_check(&g,true,&o));
    assert(!room_http_peer_guard_check(&g,false,NULL)&&g.failed);
}
static void sdk_case(const char *mode){
    idf_name_error=!strcmp(mode,"idf-name");idf_start_error=!strcmp(mode,"idf-start");idf_register_error=!strcmp(mode,"idf-register");
    idf_peer_error=!strcmp(mode,"idf-peer");idf_receive_error=!strcmp(mode,"idf-recv");idf_send_error=!strcmp(mode,"idf-send");idf_timeout=!strcmp(mode,"idf-timeout");
    room_http_driver d={0};assert(!room_http_idf_driver_init(&sdk_driver,&ap,NULL,&d));assert(room_http_idf_driver_init(&sdk_driver,&ap,trusted_peer,&d));
    room_secure_status s;assert(room_http_backend_prepare_owner(&backend,d,2,&s)&&s==ROOM_SEC_MISSING&&binds==1);
    bool opened=attached.io.open(attached.io.ctx,&ap,1);
    if(idf_name_error){assert(!opened&&!starts&&!routes_count);finish_close(1,5000);return;}
    if(idf_start_error){assert(!opened&&backend.failed&&starts==1&&!routes_count);return;}
    if(idf_register_error){assert(!opened&&routes_count==1);finish_close(1,5000);assert(atomic_load(&stops)==1);return;}
    assert(opened&&routes_count==2&&!strcmp(routes[0].uri,"/setup/handshake")&&!strcmp(routes[1].uri,"/setup/packet"));
    sdk_request=(httpd_req_t){.user_ctx=routes[0].user_ctx,.content_len=sizeof(req),.handle=&handle};
    if(!strcmp(mode,"idf-size"))sdk_request.content_len=4097;
    pthread_t th;assert(!pthread_create(&th,NULL,handler_entry,NULL));
    if(idf_peer_error||idf_receive_error||idf_timeout||!strcmp(mode,"idf-size")){
        assert(!pthread_join(th,NULL)&&handler_result==ESP_FAIL&&!atomic_load(&body_sent));
        if(idf_peer_error||!strcmp(mode,"idf-size"))assert(!atomic_load(&body_reads));
        if(idf_timeout){uint8_t b[8];size_t n;bool hs;assert(attached.io.receive(attached.io.ctx,&hs,b,sizeof(b),&n)==ROOM_PHONE_IO_ERROR);}
    }else{
        uint8_t b[8];size_t n=0;bool hs=false;room_phone_io_result rc=ROOM_PHONE_IO_WAIT;
        for(unsigned i=0;i<1000000&&rc==ROOM_PHONE_IO_WAIT;++i){rc=attached.io.receive(attached.io.ctx,&hs,b,sizeof(b),&n);sched_yield();}
        assert(rc==ROOM_PHONE_IO_OK&&hs&&n==sizeof(req)&&!memcmp(b,req,n));rc=ROOM_PHONE_IO_WAIT;
        for(unsigned i=0;i<1000000&&rc==ROOM_PHONE_IO_WAIT;++i){rc=attached.io.send(attached.io.ctx,reply,sizeof(reply),&n);sched_yield();}
        assert(!pthread_join(th,NULL));
        if(idf_send_error){assert(rc==ROOM_PHONE_IO_ERROR&&handler_result==ESP_FAIL&&!atomic_load(&body_sent));}
        else{assert(rc==ROOM_PHONE_IO_OK&&n==sizeof(reply)&&handler_result==ESP_OK&&atomic_load(&body_sent)==1&&!memcmp(received_reply,reply,n));}
    }
    finish_close(1,(uint64_t)atomic_load(&us)/1000+5000);assert(atomic_load(&stops)==1);
    for(size_t i=0;i<sizeof(sdk_driver.scratch);++i)assert(!sdk_driver.scratch[i]);
}
int main(int argc,char **argv){assert(argc==2);(void)other;if(!strcmp(argv[1],"idf-evidence"))evidence_case();else if(!strcmp(argv[1],"peer-guard"))peer_guard_case();else if(!strcmp(argv[1],"idf-restart"))sdk_restart();else if(!strncmp(argv[1],"idf-",4))sdk_case(argv[1]);else scenario(argv[1]);join_worker();printf("PASS http-backend %s\n",argv[1]);return 0;}
