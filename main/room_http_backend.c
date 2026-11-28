#include "room_http_backend.h"
#include <string.h>
enum { SLOT_IDLE, SLOT_FILL, SLOT_REQUEST, SLOT_OWNER, SLOT_REPLY, SLOT_READ, SLOT_ACK, SLOT_ERROR };
static bool owner(const room_http_backend *b) {
    return b && b->initialized && b->worker.owner==xTaskGetCurrentTaskHandle();
}
static void wipe(room_http_backend_window *w) {
    volatile uint8_t *p=w->bytes;
    for(size_t i=0;i<sizeof(w->bytes);++i)p[i]=0;
    w->length=0;
}
static bool open_window(void *ctx,const esp_netif_t *ap,uint64_t window) {
    room_http_backend *b=ctx;
    if(!owner(b)||b->failed||b->current||b->rejected_window||!window||window<=b->last_attempt)return false;
    b->last_attempt=window;
    if(!ap){b->rejected_window=window;return false;}
    room_http_backend_window *w=&b->storage;
    /* No callback can access this storage after the previous successful close.
     * This starts a new logical object lifetime; never reset a live gate. */
    memset(w,0,sizeof(*w));
    if(!room_http_prepare(&w->life,&b->worker,window)){b->failed=true;return false;}
    w->ap=ap;w->driver=b->driver;atomic_init(&w->phase,SLOT_IDLE);atomic_init(&w->stopping,false);atomic_init(&w->accepting,false);b->current=w;
    httpd_handle_t h=NULL;
    room_http_start_result r=b->driver.start(b->driver.ctx,ap,&h);
    if(r==ROOM_HTTP_START_CLEAN_FAILURE&&!h){
        if(!room_http_cancel_unstarted(&w->life)){b->failed=true;return false;}
        return false;
    }
    if(r!=ROOM_HTTP_START_READY||!h||!room_http_adopt(&w->life,h,b->driver.stop)){
        b->failed=true;(void)room_http_lock(&w->life);return false;
    }
    if(!b->driver.publish(b->driver.ctx,h,w)){
        atomic_store(&w->stopping,true);return false;
    }
    atomic_store(&w->accepting,true);return true;
}
static room_phone_io_result receive(void *ctx,bool *handshake,uint8_t *out,size_t cap,size_t *n) {
    room_http_backend *b=ctx;
    if(n)*n=0;
    if(!owner(b)||!b->current||!handshake||!out||!n)return ROOM_PHONE_IO_ERROR;
    room_http_backend_window *w=b->current;
    if(atomic_load(&w->stopping))return ROOM_PHONE_IO_ERROR;
    unsigned expected=SLOT_REQUEST;
    if(!atomic_compare_exchange_strong(&w->phase,&expected,SLOT_OWNER))
        return expected==SLOT_IDLE||expected==SLOT_FILL?ROOM_PHONE_IO_WAIT:ROOM_PHONE_IO_ERROR;
    if(!w->length||w->length>cap){atomic_store(&w->phase,SLOT_ERROR);return ROOM_PHONE_IO_ERROR;}
    *handshake=w->handshake;*n=w->length;memcpy(out,w->bytes,*n);
    return ROOM_PHONE_IO_OK;
}
static room_phone_io_result backend_send(void *ctx,const uint8_t *data,size_t n,size_t *written) {
    room_http_backend *b=ctx;
    if(written)*written=0;
    if(!owner(b)||!b->current||!written||!data||!n||n>ROOM_PHONE_PACKET_MAX)return ROOM_PHONE_IO_ERROR;
    room_http_backend_window *w=b->current;
    if(atomic_load(&w->stopping))return ROOM_PHONE_IO_ERROR;
    unsigned phase=atomic_load(&w->phase);
    if(phase==SLOT_OWNER){
        wipe(w);memcpy(w->bytes,data,n);w->length=n;
        unsigned expected=SLOT_OWNER;
        if(!atomic_compare_exchange_strong(&w->phase,&expected,SLOT_REPLY))return ROOM_PHONE_IO_ERROR;
        return ROOM_PHONE_IO_WAIT;
    }
    if(phase==SLOT_REPLY||phase==SLOT_READ)return ROOM_PHONE_IO_WAIT;
    if(phase!=SLOT_ACK||w->length!=n)return ROOM_PHONE_IO_ERROR;
    wipe(w);*written=n;atomic_store(&w->phase,SLOT_IDLE);return ROOM_PHONE_IO_OK;
}
static bool close_legacy(void *ctx){(void)ctx;return false;}
static bool close_until(void *ctx,uint64_t window,uint64_t deadline){
    room_http_backend *b=ctx;
    if(!owner(b)||b->failed)return false;
    room_http_backend_window *w=b->current;
    /* open may have rejected before owning any resources */
    if(!w){
        if(!window)return false;
        if(window==b->closed_window)return deadline==b->closed_deadline;
        if(window!=b->rejected_window)return false;
        int64_t us=esp_timer_get_time();
        if(us<0||(uint64_t)us/1000u>=deadline||deadline-(uint64_t)us/1000u>5000u){b->failed=true;return false;}
        b->rejected_window=0;b->closed_window=window;b->closed_deadline=deadline;return true;
    }
    if(w->life.window!=window)return false;
    atomic_store(&w->stopping,true);
    if(w->life.phase==HTTP_LIFE_CANCELLED){
        int64_t us=esp_timer_get_time();
        if(us<0||(uint64_t)us/1000u>=deadline||deadline-(uint64_t)us/1000u>5000u){b->failed=true;return false;}
    }else{
        room_sdk_stop_result r=room_http_close(&w->life,window,deadline);
        if(r==ROOM_SDK_LOCKED)b->failed=true;
        if(r!=ROOM_SDK_DRAINED)return false;
    }
    wipe(w);b->closed_window=window;b->closed_deadline=deadline;b->current=NULL;return true;
}
bool room_http_backend_prepare(room_http_backend *b,room_http_driver d,room_network_setup_backend *out){
    if(!out)return false;
    *out=(room_network_setup_backend){0};
    if(!b||b->initialized||b->failed||!d.start||!d.publish||!d.stop||!d.peer)return false;
    if(!room_sdk_stop_bind(&b->worker)){b->failed=true;return false;}
    b->driver=d;b->initialized=true;
    *out=(room_network_setup_backend){.io={b,open_window,receive,backend_send,close_legacy},.close_until=close_until};
    return true;
}
bool room_http_backend_offer(room_http_backend_window *w,room_http_exchange *x,const void *req,bool handshake,const uint8_t *body,size_t n){
    if(!w||!x||x->active||x->lease.gate||!req||!body||!n||n>ROOM_PHONE_PACKET_MAX)return false;
    if(!room_http_enter(&w->life,w->life.window,&x->lease))return false;
    unsigned expected=SLOT_IDLE;
    if(!atomic_load(&w->accepting)||atomic_load(&w->stopping)||!w->driver.peer(w->driver.ctx,w->ap,req)||
       !atomic_compare_exchange_strong(&w->phase,&expected,SLOT_FILL)){
        (void)room_admission_leave(&x->lease);return false;
    }
    memcpy(w->bytes,body,n);w->length=n;w->handshake=handshake;
    x->window=w;x->active=true;x->copied=false;atomic_store(&w->phase,SLOT_REQUEST);return true;
}
room_phone_io_result room_http_backend_reply(room_http_exchange *x,uint8_t *out,size_t cap,size_t *n){
    if(n)*n=0;
    if(!x||!x->active||!out||!n||x->copied)return ROOM_PHONE_IO_ERROR;
    room_http_backend_window *w=x->window;
    if(atomic_load(&w->stopping))return ROOM_PHONE_IO_ERROR;
    unsigned expected=SLOT_REPLY;
    if(!atomic_compare_exchange_strong(&w->phase,&expected,SLOT_READ))
        return expected==SLOT_REQUEST||expected==SLOT_OWNER?ROOM_PHONE_IO_WAIT:ROOM_PHONE_IO_ERROR;
    if(!w->length||w->length>cap){atomic_store(&w->phase,SLOT_ERROR);return ROOM_PHONE_IO_ERROR;}
    memcpy(out,w->bytes,w->length);*n=w->length;x->copied=true;return ROOM_PHONE_IO_OK;
}
bool room_http_backend_finish(room_http_exchange *x,bool delivered){
    if(!x||!x->active)return false;
    room_http_backend_window *w=x->window;
    unsigned expected=SLOT_READ;
    bool ok=delivered&&x->copied&&!atomic_load(&w->stopping)&&atomic_compare_exchange_strong(&w->phase,&expected,SLOT_ACK);
    if(!ok)atomic_store(&w->phase,SLOT_ERROR);
    x->active=false;x->window=NULL;
    return room_admission_leave(&x->lease)&&ok;
}

bool room_http_backend_prepare_owner(room_http_backend *b,room_http_driver d,unsigned key_id,room_secure_status *status){
    if(status)*status=ROOM_SEC_UNAVAILABLE;
    if(key_id>=6u)return false;
    room_network_setup_backend setup;
    if(!room_http_backend_prepare(b,d,&setup))return false;
    if(room_network_owner_prepare(key_id,&setup,status))return true;
    b->failed=true;return false;
}
