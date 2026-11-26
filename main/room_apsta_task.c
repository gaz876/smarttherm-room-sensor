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
static bool barrier(void *ctx,uint64_t window,bool final) {
    room_apsta_task *t=ctx;
    if (!t->pending) {
        if (t->sequence==UINT32_MAX) { t->session.locked=true; return false; }
        uint32_t token=++t->sequence;
        if (esp_event_post(ROOM_APSTA_BARRIER,0,&token,sizeof(token),0)!=ESP_OK) return false;
        t->pending=token;t->barrier_window=window;t->final_phase=final;
    }
    if (t->barrier_window!=window || t->final_phase!=final) { t->session.locked=true; return false; }
    if (atomic_load(&t->ack)!=t->pending || !room_apsta_events_poll(&t->events,&t->session)) return false;
    t->pending=0;return true;
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
    if (!owned(t) || t->pending || !room_apsta_events_poll(&t->events,&t->session)) return false;
    return room_apsta_session_open(&t->session,clock_ms,t,profile,boot,trusted,store,ssid,n,key,m,id,sdk_id);
}
bool room_apsta_task_poll(room_apsta_task *t) {
    if (!owned(t)) return false;
    bool drained=room_apsta_events_poll(&t->events,&t->session);
    return room_apsta_session_step(&t->session,drained);
}
bool room_apsta_task_packet(room_apsta_task *t,bool hs,const uint8_t *in,size_t len,const uint8_t **out,size_t *n) {
    if (!out || !n) return false;
    *out=NULL;*n=0;
    if (!owned(t) || !room_apsta_events_poll(&t->events,&t->session) ||
        !room_apsta_session_step(&t->session,true) || !room_apsta_ready(&t->session.radio,t->session.bridge.window)) return false;
    return room_sec2_transport_exchange(&t->session.transport,hs,in,len,out,n);
}
bool room_apsta_task_sent(room_apsta_task *t,bool ok) {
    return owned(t) && room_sec2_transport_finish(&t->session.transport,ok);
}
bool room_apsta_task_close(room_apsta_task *t,bool sender_drained) {
    if (!owned(t)) return false;
    return room_apsta_session_close(&t->session,sender_drained,barrier,t);
}
