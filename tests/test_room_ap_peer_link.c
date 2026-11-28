#define main event_fixture_main
#include "test_room_ap_peer_events.c"
#undef main
#include "room_wifi_apsta_link.h"
#include "room_wifi_secure_store.h"
#include "room_http_idf_peer.h"
#include "esp_netif.h"
#include "lwip/sockets.h"
static room_wifi_setup_io setup;
static room_apsta_task *task;
static room_phone_io attached_io;
static unsigned opens,polls,closes,radio_closes;
static bool invalid_on_open;
static int queue_marker;
int64_t esp_timer_get_time(void){return 1000000;}
bool room_wifi_idf_is_owner(void){return true;}
bool room_wifi_idf_init_missing(void){return false;}
bool room_wifi_idf_bind_setup(room_wifi_setup_io io){setup=io;return io.bind(io.ctx);}
static bool ready(void *ctx){(void)ctx;return true;}
room_secure_status room_secure_load(room_enroll_profile *p){*p=(room_enroll_profile){0};return ROOM_SEC_VALID;}
room_enroll_store room_secure_store(void){return (room_enroll_store){NULL,ready};}
bool room_apsta_task_bind(room_apsta_task *t){task=t;t->ready=room_apsta_events_bind(&t->events,&queue_marker);return t->ready;}
bool room_apsta_task_open(room_apsta_task *t,phone_profile_state profile,bool boot,bool trusted,const room_enroll_store *store,const uint8_t *s,size_t n,const uint8_t *k,size_t m,const protocomm_security2_params_t *id,uint32_t serial){
    (void)profile;(void)boot;(void)store;(void)s;(void)n;(void)k;(void)m;(void)id;(void)serial;assert(trusted&&room_apsta_events_begin(&t->events));
    t->session.radio.ap=&ap;t->session.radio.phase=APSTA_RUNNING;t->session.bridge.window=++t->session.bridge.last_window;t->session.bridge.phone.deadline=3000;
    associate();if(invalid_on_open)emit(WIFI_EVENT_AP_STACONNECTED,&station);return true;
}
bool room_apsta_task_poll(room_apsta_task *t){assert(t==task);++polls;return true;}
bool room_apsta_task_close_until(room_apsta_task *t,bool drained,uint64_t deadline){assert(t==task&&drained&&deadline==5000);++radio_closes;room_apsta_events_end(&t->events);t->session.radio.phase=APSTA_IDLE;t->session.bridge.window=0;return true;}
static uint64_t linked_epoch(void){uint64_t token=UINT64_MAX;bool ok=room_wifi_apsta_link_peer_epoch(NULL,&ap,station.mac,&token);assert(ok==(token!=0));return token;}
static bool io_open(void *v,const esp_netif_t *n,uint64_t w){(void)v;assert(n==&ap&&w&&linked_epoch());++opens;return true;}
static int io_recv(void *v,bool *hs,uint8_t *b,size_t n,size_t *out){(void)v;(void)hs;(void)b;(void)n;(void)out;return 0;}
static int io_send(void *v,const uint8_t *b,size_t n,size_t *out){(void)v;(void)b;(void)n;(void)out;return 0;}
static bool io_close(void *v){(void)v;return false;}
static bool close_timed(void *v,uint64_t w,uint64_t d){(void)v;assert(w&&d==5000&&!linked_epoch());++closes;return true;}
bool room_phone_channel_open(room_phone_channel *c,room_apsta_task *t,room_phone_io io){assert(t==task&&!c->attempted);c->window=t->session.bridge.window;c->attempted=true;attached_io=io;return io.open(io.ctx,t->session.radio.ap,c->window);}
bool room_phone_channel_poll(room_phone_channel *c){assert(c->attempted);++polls;return true;}
bool room_phone_channel_close_until(room_phone_channel *c,uint64_t d,room_phone_clock clk,void *ctx,room_phone_timed_close fn){(void)clk;(void)ctx;assert(!linked_epoch());if(c->attempted){assert(fn(attached_io.ctx,c->window,d));*c=(room_phone_channel){0};}return true;}
/* Local observations are synthetic; no physical interface or sockets. */
bool esp_netif_is_netif_up(esp_netif_t *n){return n==&ap;}
int esp_netif_get_ip_info(esp_netif_t *n,esp_netif_ip_info_t *ip){assert(n==&ap);ip->ip.addr=1;return ESP_OK;}
int esp_netif_dhcps_get_status(esp_netif_t *n,esp_netif_dhcp_status_t *d){assert(n==&ap);*d=ESP_NETIF_DHCP_STARTED;return ESP_OK;}
int esp_netif_dhcps_get_clients_by_mac(esp_netif_t *n,size_t count,esp_netif_pair_mac_ip_t *pair){assert(n==&ap&&count==1&&!memcmp(pair->mac,station.mac,6));pair->ip.addr=2;return ESP_OK;}
int esp_wifi_ap_get_sta_list(wifi_sta_list_t *list){list->num=1;memcpy(list->sta[0].mac,station.mac,6);return ESP_OK;}
int httpd_req_to_sockfd(httpd_req_t *r){return r->fd;}
static int endpoint(int fd,struct sockaddr *addr,socklen_t *n,uint32_t ip){assert(fd==7&&*n==sizeof(struct sockaddr_in));struct sockaddr_in a={.sin_family=AF_INET,.sin_addr={.s_addr=ip}};memcpy(addr,&a,sizeof(a));return 0;}
int mock_getsockname(int fd,struct sockaddr *a,socklen_t *n){return endpoint(fd,a,n,1);}
int mock_getpeername(int fd,struct sockaddr *a,socklen_t *n){return endpoint(fd,a,n,2);}
static bool observe(room_http_idf_peer *p,room_http_peer_guard *g){httpd_req_t req={7};room_http_peer_observation o={0};bool ok=room_http_idf_peer_observe(p,&ap,&req,&o);return room_http_peer_guard_check(g,ok,&o);}
static bool prepare(void *ctx,room_wifi_apsta_window *w){(void)ctx;w->ssid_len=1;w->key_len=16;w->ssid[0]='x';memset(w->key,'x',16);w->salt_len=8;w->verifier_len=sizeof(w->verifier);w->sdk_id=1;return true;}
int main(void){room_wifi_apsta_provider p={NULL,ready,prepare};room_phone_io io={NULL,io_open,io_recv,io_send,io_close};room_wifi_setup_result result;
    assert(room_wifi_apsta_link_bind(p)&&!linked_epoch());assert(room_wifi_apsta_link_attach_io_until(io,close_timed));
    room_http_idf_peer peer=room_http_idf_peer_local();
    uint64_t last=0;for(unsigned i=0;i<32;++i){room_http_peer_guard guard={0};assert(setup.start(NULL,2000,ROOM_WIFI_PROFILE_VALID,true));uint64_t v=linked_epoch();assert(v>last);last=v;assert(observe(&peer,&guard)&&guard.association==v);assert(setup.poll(NULL));emit(WIFI_EVENT_AP_STADISCONNECTED,NULL);assert(!linked_epoch());assert(!observe(&peer,&guard)&&guard.failed);assert(!setup.poll(NULL));assert(setup.stop(NULL,5000,&result)&&result==ROOM_WIFI_SETUP_NO_SAVE);assert(!linked_epoch());}
    assert(opens==32&&polls==32&&closes==32&&radio_closes==32);invalid_on_open=true;assert(!setup.start(NULL,2000,ROOM_WIFI_PROFILE_VALID,true));assert(!linked_epoch()&&opens==32);assert(setup.stop(NULL,5000,&result)&&radio_closes==33&&closes==32);
    assert(!pthread_mutex_destroy(&task->events.peer_lock));puts("PASS: 32 actual link lifetimes; attach before channel open, revoked before poll/close, ambiguous start rejected; real HTTP observer sees revocation");}
