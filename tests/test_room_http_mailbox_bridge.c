#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "room_http_async.h"
#include "freertos/semphr.h"
#include "completion_adapter.h"

/* Offline integration of the existing ack mailbox, HTTP lease and actual
 * SDK-stop worker. SDK/RTOS calls are mocked; no real command or listener.
 * One transaction per window. Semaphore handoffs belong to the fixture;
 * this does not establish the final production scheduling contract. */
typedef enum { NORMAL, SEND_FAIL, SDK_FAIL, LATE, CLOSE_PENDING } mode;
typedef struct {
    room_http_lifecycle life, next;
    room_http_async async;
    room_sdk_stop worker;
    ack_adapter box;
    httpd_req_t original;
    SemaphoreHandle_t submitted, published, completing, proceed, completed, allow_ack, done;
    uint64_t window, ticket;
    mode test;
    atomic_uint stops, completes;
} fixture;
static fixture cases[5];
static void take(SemaphoreHandle_t s){assert(xSemaphoreTake(s,pdMS_TO_TICKS(3000))==pdTRUE);}
static void give(SemaphoreHandle_t s){assert(xSemaphoreGive(s)==pdTRUE);}
static uint64_t now(void){int64_t us=esp_timer_get_time();assert(us>=0);return (uint64_t)us/1000u;}
static bool read_clock(void *ctx,uint64_t *ms){(void)ctx;*ms=now();return true;}
static ack_result event(fixture *f,mb_event e,bool receipt,mb_reply *out){
    e.window=f->window;
    return ack_step(&f->box,e,receipt,out,read_clock,NULL);
}
static ack_result receipt(fixture *f,mb_reply *out){return event(f,(mb_event){.ticket=f->ticket},true,out);}
static ack_result simple(fixture *f,mb_action a,bool flag,bool drained){mb_reply out;return event(f,(mb_event){.action=a,.flag=flag,.drained=drained},false,&out);}
esp_err_t httpd_req_async_handler_begin(httpd_req_t *r,httpd_req_t **out){*out=malloc(sizeof **out);assert(*out);**out=*r;return ESP_OK;}
esp_err_t httpd_req_async_handler_complete(httpd_req_t *r){
    fixture *f=r->handle;assert(r!=&f->original);atomic_fetch_add(&f->completes,1);free(r);
    give(f->completing);take(f->proceed);return f->test==SDK_FAIL?-1:ESP_OK;
}
static esp_err_t stop_mock(httpd_handle_t h){
    fixture *f=h;assert(room_admission_drained(&f->life.gate,f->window));
    assert(atomic_load(&f->completes)==1);atomic_fetch_add(&f->stops,1);return ESP_OK;
}
static void callback(void *ctx){
    fixture *f=ctx;room_lease cb={0},transit={0};mb_reply out;
    uint8_t input[3]={1,2,3},reply[3]={0};
    assert(room_http_enter(&f->life,f->window,&cb));
    assert(room_http_enter(&f->life,f->window,&transit));
    assert(room_http_async_begin(&f->async,&f->life,f->window,&cb,&f->original));
    assert(event(f,(mb_event){.action=MB_SUBMIT,.input=input,.size=sizeof input},false,&out)==ACK_OK);
    f->ticket=out.ticket;memset(input,0,sizeof input);
    assert(room_admission_leave(&cb));give(f->submitted);
    take(f->published);
    assert(event(f,(mb_event){.action=MB_TAKE_REPLY,.ticket=f->ticket,.output=reply,.size=sizeof reply},false,&out)==ACK_OK);
    assert(out.length==sizeof reply&&reply[0]==4&&reply[1]==5&&reply[2]==6);
    /* The fixture simulates last access to the copied reply, then SDK complete.
     * Receipt publication occurs after complete returns, under transit lease. */
    memset(reply,0,sizeof reply);
    bool completed=room_http_async_complete(&f->async);
    assert(completed==(f->test!=SDK_FAIL));
    give(f->completed);take(f->allow_ack);
    bool sent=completed&&f->test!=SEND_FAIL;
    ack_result rc=event(f,(mb_event){.action=MB_SENT,.ticket=f->ticket,.flag=sent},false,&out);
    assert(rc==(f->test==NORMAL?ACK_OK:ACK_REJECT));
    assert(!room_http_async_complete(&f->async));
    assert(room_admission_leave(&transit));give(f->done);vTaskDelete(NULL);
}
static void scenario(fixture *f,unsigned w,mode m){
    f->window=w;f->test=m;f->original.handle=f;ack_init(&f->box);
    f->submitted=xSemaphoreCreateBinary();f->published=xSemaphoreCreateBinary();
    f->completing=xSemaphoreCreateBinary();f->proceed=xSemaphoreCreateBinary();f->done=xSemaphoreCreateBinary();
    f->completed=xSemaphoreCreateBinary();f->allow_ack=xSemaphoreCreateBinary();
    assert(f->submitted&&f->published&&f->completing&&f->proceed&&f->done&&f->completed&&f->allow_ack);
    assert(room_sdk_stop_bind(&f->worker));assert(room_http_begin(&f->life,&f->worker,w,f,stop_mock));
    assert(simple(f,MB_OPEN,false,false)==ACK_OK);assert(simple(f,MB_STARTED,true,false)==ACK_OK);
    room_lease owner_lease={0};assert(room_http_enter(&f->life,w,&owner_lease));
    assert(xTaskCreate(callback,"bridge",4096,f,3,NULL)==pdPASS);take(f->submitted);
    mb_reply out;uint8_t req[3]={0},resp[3]={4,5,6},other[1]={9};
    assert(event(f,(mb_event){.action=MB_TAKE_REQUEST,.ticket=f->ticket,.output=req,.size=sizeof req},false,&out)==ACK_OK);
    assert(out.length==3&&req[0]==1&&req[1]==2&&req[2]==3);
    memset(req,0,sizeof req);
    assert(event(f,(mb_event){.action=MB_TAKE_REQUEST,.ticket=f->ticket,.output=req,.size=sizeof req},false,&out)==ACK_REJECT);
    assert(event(f,(mb_event){.action=MB_PUBLISH,.ticket=f->ticket,.input=resp,.size=sizeof resp},false,&out)==ACK_OK);
    memset(resp,0,sizeof resp);give(f->published);take(f->completing);
    assert(receipt(f,&out)==ACK_WAIT&&out.ticket==0);
    assert(event(f,(mb_event){.action=MB_SUBMIT,.input=other,.size=1},false,&out)==ACK_REJECT);
    assert(!atomic_load(&f->stops));
    uint64_t deadline=now()+((m==LATE||m==SDK_FAIL)?300u:1500u);
    if(m==LATE||m==CLOSE_PENDING){
        assert(simple(f,MB_STOP,false,false)==ACK_OK);
        assert(room_http_close(&f->life,w,deadline)==ROOM_SDK_WAIT);
        assert(!room_admission_drained(&f->life.gate,w));
        assert(simple(f,MB_DRAINED,false,false)==ACK_REJECT);
        if(m==LATE){while(now()<deadline)vTaskDelay(1);assert(room_http_close(&f->life,w,deadline)==ROOM_SDK_LOCKED);}
    }
    give(f->proceed);take(f->completed);
    assert(receipt(f,&out)==((m==LATE||m==CLOSE_PENDING)?ACK_REJECT:ACK_WAIT));
    assert(!atomic_load(&f->stops));
    if(m==CLOSE_PENDING)assert(room_http_close(&f->life,w,deadline)==ROOM_SDK_WAIT);
    give(f->allow_ack);take(f->done);
    if(m==NORMAL){assert(receipt(f,&out)==ACK_OK&&out.ticket==f->ticket);assert(receipt(f,&out)==ACK_REJECT);}
    else assert(receipt(f,&out)==ACK_REJECT);
    assert(simple(f,MB_STOP,false,false)==ACK_OK);
    assert(room_admission_leave(&owner_lease));
    room_sdk_stop_result rc=room_http_close(&f->life,w,deadline);
    uint64_t limit=now()+2000u;while(rc==ROOM_SDK_WAIT){assert(now()<limit);vTaskDelay(1);rc=room_http_close(&f->life,w,deadline);}
    bool locked=m==LATE||m==SDK_FAIL;
    assert(rc==(locked?ROOM_SDK_LOCKED:ROOM_SDK_DRAINED));
    assert(atomic_load(&f->stops)==(locked?0u:1u));assert(atomic_load(&f->completes)==1);
    /* Model's own drain proof may not override the actual SDK stop result. */
    assert(simple(f,MB_DRAINED,!locked,!locked)==(locked?ACK_REJECT:ACK_OK));
    assert(f->worker.quarantined==locked);
    mb_reply tmp;ack_result reopen=ack_step(&f->box,(mb_event){.action=MB_OPEN,.window=w+100},false,&tmp,read_clock,NULL);
    assert(reopen==(locked?ACK_REJECT:ACK_OK));
    assert(room_http_prepare(&f->next,&f->worker,w+100)==!locked);
    if(!locked){
        assert(room_http_cancel_unstarted(&f->next));
        assert(ack_step(&f->box,(mb_event){.window=w+100,.ticket=f->ticket},true,&tmp,read_clock,NULL)==ACK_REJECT);
    }
    printf("HTTP_MAILBOX_BRIDGE_CASE %u mode=%d result=%d stops=%u\n",w,(int)m,(int)rc,atomic_load(&f->stops));
}
void app_main(void){for(unsigned i=0;i<5;i++)scenario(&cases[i],i+1,(mode)i);puts("HTTP_MAILBOX_BRIDGE_PASS");}
