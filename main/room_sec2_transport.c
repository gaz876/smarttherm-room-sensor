#include "room_sec2_transport.h"
#include <stdlib.h>

static void stop(room_sec2_transport *t) {
    t->stopped=true;
    if (t->bridge) bridge_stop(t->bridge);
}
static void clear_reply(room_sec2_transport *t) {
    free(t->reply); t->reply=NULL; t->reply_len=0; t->pending=false;
}
/* Called only through the SDK's authenticated request path; not public I/O. */
static esp_err_t command(uint32_t id,const uint8_t *in,ssize_t len,
    uint8_t **out,ssize_t *n,void *v) {
    room_sec2_transport *t=v;
    if (!out || !n) return ESP_FAIL;
    *out=NULL; *n=0;
    if (!t || !t->dispatching || t->stopped || t->ticket || id!=t->sec.session ||
        !in || len<2 || len>99) return ESP_FAIL;
    if (!t->claimed) {
        if (!bridge_claim(t->bridge,t->clock(t->ctx),t->window,t->generation)) return ESP_FAIL;
        t->claimed=true;
    }
    uint8_t *p=malloc(ROOM_SETUP_REPLY_SIZE);
    if (!p) return ESP_FAIL;
    t->ticket=bridge_request(t->bridge,t->window,t->generation,in,(size_t)len,p,
        t->store,t->clock,t->live,t->ctx);
    if (!t->ticket) { free(p); return ESP_FAIL; }
    *out=p; *n=ROOM_SETUP_REPLY_SIZE; return ESP_OK;
}
bool room_sec2_transport_start(room_sec2_transport *t,room_setup_bridge *b,
    const protocomm_security2_params_t *identity,uint32_t id,uint64_t generation,
    const room_enroll_store *store,room_setup_clock clock,room_setup_live live,void *ctx) {
    if (!t || t->bridge || t->sec.pc || t->sec.failed || !b || !b->window || b->stopping ||
        !b->phone.window || b->phone.session || !generation || !clock || !enroll_ready(store)) return false;
    t->bridge=b; t->window=b->window; t->generation=generation;
    t->store=store; t->clock=clock; t->live=live; t->ctx=ctx;
    if (!room_sec2_create(&t->sec,identity,command,t) || !room_sec2_open(&t->sec,id)) {
        stop(t); return false;
    }
    return true;
}
/* Input is an SDK packet, not a plaintext command. Output is borrowed until
 * finish/destroy. Do not call again while any output is outstanding. */
bool room_sec2_transport_exchange(room_sec2_transport *t,bool handshake,
    const uint8_t *in,size_t len,const uint8_t **out,size_t *n) {
    if (!out || !n) return false;
    *out=NULL; *n=0;
    if (!t || !t->sec.pc || !t->bridge || t->stopped || t->pending || t->dispatching) return false;
    uint64_t now=t->clock(t->ctx); bridge_tick(t->bridge,now);
    if (t->bridge->stopping || !t->bridge->phone.window || !in ||
        (handshake ? (t->claimed || !len || len>4096) : (len<18 || len>115))) {
        stop(t); return false;
    }
    t->deadline=setup_life_limit(now); t->ticket=0;
    t->dispatching=!handshake;
    uint8_t *p=NULL; ssize_t m=0;
    esp_err_t rc=protocomm_req_handle(t->sec.pc,handshake?"room-session":"room-command",
        t->sec.session,in,(ssize_t)len,&p,&m);
    t->dispatching=false;
    uint64_t end=t->clock(t->ctx); bridge_tick(t->bridge,end);
    if (rc!=ESP_OK || !p || m<=0 || m>4096 || end<now || end>=t->deadline ||
        t->bridge->stopping || (!handshake && (!t->ticket || m!=27))) {
        free(p); stop(t); return false;
    }
    t->reply=p; t->reply_len=(size_t)m; t->pending=true; *out=p; *n=(size_t)m; return true;
}
/* Must be called after the actual sender has finished using the buffer.
 * Success means local send completion, not proof of peer receipt. */
bool room_sec2_transport_finish(room_sec2_transport *t,bool sent) {
    if (!t || !t->pending) return false;
    uint64_t now=t->clock(t->ctx); bridge_tick(t->bridge,now);
    bool ok=sent && now<t->deadline && !t->bridge->stopping;
    if (t->ticket) ok=bridge_sent(t->bridge,now,t->window,t->generation,t->ticket,ok) && ok;
    clear_reply(t); t->ticket=0;
    if (!ok || t->bridge->stopping) stop(t);
    return ok;
}
bool room_sec2_transport_destroy(room_sec2_transport *t,bool drained) {
    if (!t || !drained || t->dispatching) return false;
    if (!t->bridge) return !t->sec.pc;
    stop(t); clear_reply(t);
    if (!room_sec2_destroy(&t->sec,true)) return false;
    if (!bridge_drained(t->bridge,t->clock(t->ctx),t->window)) return false;
    *t=(room_sec2_transport){0}; return true;
}
