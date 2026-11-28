#include "room_http_idf_driver.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include <net/if.h>
#include <string.h>

/* No unbounded retry on socket timeout; the clock check bounds each loop,
 * but SDK blocking calls still require real target timing qualification. */
static bool clock_ms(uint64_t *out){
    int64_t us=esp_timer_get_time();if(us<0)return false;
    *out=(uint64_t)us/1000u;return true;
}
static bool within(uint64_t *last,uint64_t start,uint64_t budget){
    uint64_t now;
    if(!clock_ms(&now)||now<*last||now-start>=budget)return false;
    *last=now;return true;
}
static bool peer(void *ctx,const esp_netif_t *ap,const void *request){
    room_http_idf_driver *d=ctx;
    if(!d||!d->initialized||!ap||!request)return false;
    room_http_peer_observation o={0};
    bool observed=d->observe(d->peer_ctx,ap,request,&o);
    return room_http_peer_guard_check(&d->guard,observed,&o);
}
static room_http_start_result start(void *ctx,const esp_netif_t *ap,httpd_handle_t *h){
    room_http_idf_driver *d=ctx;
    if(!h)return ROOM_HTTP_START_CLEAN_FAILURE;
    *h=NULL;
    if(!d||!d->initialized||!ap)return ROOM_HTTP_START_CLEAN_FAILURE;
    struct ifreq name={0};
    /* SDK getter accepts a writable netif pointer; it only reads the name. */
    if(esp_netif_get_netif_impl_name((esp_netif_t *)ap,name.ifr_name)!=ESP_OK||
       !name.ifr_name[0]||!memchr(name.ifr_name,0,sizeof(name.ifr_name)))return ROOM_HTTP_START_CLEAN_FAILURE;
    /* Previous SDK stop joined all callbacks before this start. */
    d->guard=(room_http_peer_guard){0};
    httpd_config_t config=HTTPD_DEFAULT_CONFIG();
    config.if_name=&name;config.max_open_sockets=1;config.max_uri_handlers=2;
    config.recv_wait_timeout=1;config.send_wait_timeout=1;config.lru_purge_enable=false;
    /* Do not infer cleanup from a nonzero SDK start result. */
    if(httpd_start(h,&config)!=ESP_OK||!*h)return ROOM_HTTP_START_UNCERTAIN;
    return ROOM_HTTP_START_READY;
}
static esp_err_t exchange(httpd_req_t *req,bool handshake){
    if(!req||!req->user_ctx)return ESP_FAIL;
    room_http_backend_window *w=req->user_ctx;
    room_lease callback={0};
    if(!room_http_enter(&w->life,w->life.window,&callback))return ESP_FAIL;
    room_http_idf_driver *d=w->driver.ctx;
    room_http_exchange x={0};esp_err_t result=ESP_FAIL;
    uint64_t t=0,last=0;
    /* Single HTTPD task owns scratch. No pointer is passed to the Wi-Fi task. */
    if(req->handle!=w->life.handle||!atomic_load(&w->accepting)||atomic_load(&w->stopping)||!peer(d,w->ap,req)||!req->content_len||
       req->content_len>sizeof(d->scratch)||!clock_ms(&t))goto done;
    last=t;size_t off=0;
    while(off<req->content_len){
        if(atomic_load(&w->stopping)||!within(&last,t,2000u))goto done;
        int n=httpd_req_recv(req,(char *)d->scratch+off,req->content_len-off);
        if(n<=0||(size_t)n>req->content_len-off)goto done;
        off+=(size_t)n;
    }
    if(!within(&last,t,2000u)||!room_http_backend_offer(w,&x,req,handshake,d->scratch,off))goto done;
    for(;;){
        if(!within(&last,t,3000u)||!peer(d,w->ap,req))goto done;
        size_t n=0;room_phone_io_result r=room_http_backend_reply(&x,d->scratch,sizeof(d->scratch),&n);
        if(r==ROOM_PHONE_IO_ERROR)goto done;
        if(r==ROOM_PHONE_IO_OK){
            if(!n||atomic_load(&w->stopping)||!peer(d,w->ap,req))goto done;
            if(httpd_resp_set_type(req,"application/octet-stream")!=ESP_OK||
               httpd_resp_set_hdr(req,"Cache-Control","no-store")!=ESP_OK||
               httpd_resp_send(req,(const char *)d->scratch,n)!=ESP_OK)goto done;
            if(!within(&last,t,3000u)||!peer(d,w->ap,req))goto done;
            if(room_http_backend_finish(&x,true))result=ESP_OK;
            break;
        }
        TickType_t ticks=pdMS_TO_TICKS(10u);vTaskDelay(ticks?ticks:1);
    }
done:
    if(x.active)(void)room_http_backend_finish(&x,false);
    if(d&&d->guard.failed)atomic_store(&w->stopping,true);
    if(d){volatile uint8_t *p=d->scratch;for(size_t i=0;i<sizeof(d->scratch);++i)p[i]=0;}
    if(!room_admission_leave(&callback))result=ESP_FAIL;
    return result;
}
static esp_err_t handshake(httpd_req_t *r){return exchange(r,true);}
static esp_err_t packet(httpd_req_t *r){return exchange(r,false);}
static bool publish(void *ctx,httpd_handle_t h,room_http_backend_window *w){
    if(!ctx||!h||!w)return false;
    const httpd_uri_t a={.uri="/setup/handshake",.method=HTTP_POST,.handler=handshake,.user_ctx=w};
    const httpd_uri_t b={.uri="/setup/packet",.method=HTTP_POST,.handler=packet,.user_ctx=w};
    return httpd_register_uri_handler(h,&a)==ESP_OK&&httpd_register_uri_handler(h,&b)==ESP_OK;
}
bool room_http_idf_driver_init(room_http_idf_driver *d,void *ctx,room_http_peer_observe observe,room_http_driver *out){
    if(!out)return false;
    *out=(room_http_driver){0};
    if(!d||d->initialized||!observe)return false;
    d->peer_ctx=ctx;d->observe=observe;d->initialized=true;
    *out=(room_http_driver){d,start,publish,httpd_stop,peer};return true;
}
