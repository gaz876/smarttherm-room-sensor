#include "room_apsta_task.h"
#include "esp_timer.h"

ESP_EVENT_DEFINE_BASE(ROOM_APSTA_BARRIER);
static uint64_t clock_ms(void *ctx) { (void)ctx; return (uint64_t)(esp_timer_get_time()/1000); }
static bool owned(const room_apsta_task *t) {
    return t && t->ready && !t->failed && t->owner==xTaskGetCurrentTaskHandle();
}
static void barrier_received(void *ctx,esp_event_base_t base,int32_t id,void *data) {
    room_apsta_task *t=ctx;
    (void)base;(void)id;
    if (data) { uint32_t token;memcpy(&token,data,sizeof(token));atomic_store(&t->ack,token); }
}
static bool barrier_wait(room_apsta_task *t,uint64_t window,bool final,bool pretrial) {
    if (!t->pending) {
        if (t->sequence==UINT32_MAX) { t->session.locked=true; return false; }
        uint32_t token=++t->sequence;
        if (esp_event_post(ROOM_APSTA_BARRIER,0,&token,sizeof(token),0)!=ESP_OK) return false;
        t->pending=token;t->barrier_window=window;t->final_phase=final;t->pretrial_phase=pretrial;
    }
    if (t->barrier_window!=window || t->final_phase!=final || t->pretrial_phase!=pretrial) {
        t->session.locked=true; return false;
    }
    if (atomic_load(&t->ack)!=t->pending) return false;
    bool drained=pretrial?room_apsta_events_poll(&t->events,&t->session):room_apsta_events_discard(&t->events);
    if (!drained) return false;
    t->pending=0;t->pretrial_phase=false;return true;
}
static bool barrier(void *ctx,uint64_t window,bool final) {
    room_apsta_task *t=ctx;
    /* An aborted pretrial marker is not a teardown marker. Retire it before
     * posting a new one after stop; never reuse its acknowledgement. */
    if (t->pending && t->pretrial_phase) {
        if (t->barrier_window!=window) { t->session.locked=true; return false; }
        if (atomic_load(&t->ack)!=t->pending) return false;
        t->pending=0;t->pretrial_phase=false;
    }
    return barrier_wait(t,window,final,false);
}
static bool advance(room_apsta_task *t,bool drained) {
    room_apsta_session *s=&t->session;
    /* Process deadlines even while waiting for the asynchronous marker. */
    if (!s->radio.trial && s->bridge.phone.enrollment.state!=ENROLL_IDLE) {
        drained=drained && !s->locked && s->bridge.window && !s->bridge.stopping &&
            !s->transport.stopped && !s->transport.pending &&
            s->bridge.phone.enrollment.state==ENROLL_TRIAL &&
            room_apsta_ready(&s->radio,s->bridge.window) &&
            barrier_wait(t,s->bridge.window,false,true);
    }
    return room_apsta_session_step(s,drained);
}
bool room_apsta_task_bind(room_apsta_task *t,QueueHandle_t queue) {
    if (!t || !queue || t->owner || t->failed) return false;
    t->owner=xTaskGetCurrentTaskHandle();atomic_init(&t->ack,0u);
    if (!t->owner || esp_event_handler_instance_register(ROOM_APSTA_BARRIER,0,barrier_received,t,&t->barrier_handler)!=ESP_OK ||
        !room_apsta_events_bind(&t->events,queue)) { t->failed=true; return false; }
    t->ready=true;return true;
}
bool room_apsta_task_open(room_apsta_task *t,phone_profile_state profile,bool boot,bool trusted,
    const room_enroll_store *store,const uint8_t *ssid,size_t n,const uint8_t *key,size_t m,
    const protocomm_security2_params_t *id,uint32_t sdk_id) {
    if (!owned(t) || t->pending || t->session.locked || t->session.bridge.window ||
        !trusted || !room_apsta_events_begin(&t->events)) return false;
    bool ok=room_apsta_session_open(&t->session,clock_ms,t,profile,boot,trusted,store,ssid,n,key,m,id,sdk_id);
    if (!ok && !t->session.bridge.window) room_apsta_events_end(&t->events);
    return ok;
}
bool room_apsta_task_poll(room_apsta_task *t) {
    if (!owned(t)) return false;
    return advance(t,room_apsta_events_poll(&t->events,&t->session));
}
bool room_apsta_task_packet(room_apsta_task *t,bool hs,const uint8_t *in,size_t len,const uint8_t **out,size_t *n) {
    if (!out || !n) return false;
    *out=NULL;*n=0;
    if (!owned(t) || !room_apsta_events_poll(&t->events,&t->session) ||
        !advance(t,true) || t->pending || !room_apsta_ready(&t->session.radio,t->session.bridge.window)) return false;
    return room_sec2_transport_exchange(&t->session.transport,hs,in,len,out,n);
}
bool room_apsta_task_sent(room_apsta_task *t,bool ok) {
    return owned(t) && room_sec2_transport_finish(&t->session.transport,ok);
}
bool room_apsta_task_close(room_apsta_task *t,bool sender_drained) {
    if (!owned(t)) return false;
    bool ok=room_apsta_session_close(&t->session,sender_drained,barrier,t);
    if (ok) room_apsta_events_end(&t->events);
    return ok;
}
