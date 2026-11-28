#include <assert.h>
#include <stdio.h>
#include "room_http_lifecycle.h"

/* Deterministic SDK boundary stub: no server, network, or device. */
static uint64_t ms=100;
static unsigned requests,polls,checks;
static bool reject;
static room_sdk_stop_result result=ROOM_SDK_WAIT;
static int owner_token,other_token,handle_token;
static TaskHandle_t task;
#define CHECK(x) do{++checks;assert(x);}while(0)
TaskHandle_t xTaskGetCurrentTaskHandle(void){return task;}
int64_t esp_timer_get_time(void){return (int64_t)ms*1000;}
static esp_err_t stop_stub(httpd_handle_t h){(void)h;return ESP_OK;}
bool room_sdk_stop_reserve(room_sdk_stop *s,const void *token,uint64_t w){
    if(!s||s->owner!=task||s->quarantined||!token||s->reservation||!w||w<=s->window)return false;
    s->reservation=token;s->reserved_window=w;return true;
}
bool room_sdk_stop_cancel_unstarted(room_sdk_stop *s,const void *token,uint64_t w){
    if(s->owner!=task||s->quarantined||!token||s->reservation!=token||s->reserved_window!=w||w<=s->window)return false;
    s->window=w;s->reservation=NULL;s->reserved_window=0;return true;
}
bool room_sdk_stop_request_for(room_sdk_stop *s,const void *token,uint64_t w,uint64_t d,httpd_handle_t h,room_sdk_stop_fn fn){
    CHECK(s->reservation==token&&s->reserved_window==w);
    ++requests;CHECK(h==&handle_token&&fn==stop_stub&&d>ms&&w>s->window);
    if(reject)return false;
    s->window=w;return true;
}
room_sdk_stop_result room_sdk_stop_poll_for(room_sdk_stop *s,const void *token,uint64_t w,bool drained){
    ++polls;CHECK(s->reservation==token&&w==s->window&&drained);
    if(result==ROOM_SDK_DRAINED){s->reservation=NULL;s->reserved_window=0;}return result;
}
static room_http_lifecycle cases[7];
static room_sdk_stop workers[7];
static void setup(unsigned i){
    task=(TaskHandle_t)(void*)&owner_token;
    workers[i].owner=task;workers[i].worker=(TaskHandle_t)(void*)&other_token;
    ms=100;requests=polls=0;reject=false;result=ROOM_SDK_WAIT;
    CHECK(room_http_begin(&cases[i],&workers[i],i+1,&handle_token,stop_stub));
}
int main(void){
    setup(0);room_lease a={0},b={0},denied={0};
    CHECK(room_http_enter(&cases[0],1,&a));CHECK(room_http_enter(&cases[0],1,&b));
    CHECK(room_http_close(&cases[0],1,200)==ROOM_SDK_WAIT&&requests==0);
    CHECK(!room_http_enter(&cases[0],1,&denied));CHECK(room_admission_leave(&a));
    CHECK(room_http_close(&cases[0],1,200)==ROOM_SDK_WAIT&&requests==0);
    CHECK(room_admission_leave(&b));
    CHECK(room_http_close(&cases[0],1,200)==ROOM_SDK_WAIT&&requests==1&&polls==1);
    CHECK(cases[0].handle==NULL);result=ROOM_SDK_DRAINED;
    CHECK(room_http_close(&cases[0],1,200)==ROOM_SDK_DRAINED);
    CHECK(room_http_close(&cases[0],1,200)==ROOM_SDK_DRAINED&&requests==1&&polls==2);
    CHECK(!room_http_begin(&cases[0],&workers[0],9,&handle_token,stop_stub));
    CHECK(room_http_close(&cases[0],1,201)==ROOM_SDK_LOCKED&&!workers[0].quarantined);
    setup(1);CHECK(room_http_enter(&cases[1],2,&a));
    CHECK(room_http_close(&cases[1],2,200)==ROOM_SDK_WAIT);ms=200;
    CHECK(room_http_close(&cases[1],2,200)==ROOM_SDK_LOCKED&&requests==0);
    CHECK(room_admission_leave(&a));CHECK(room_http_close(&cases[1],2,300)==ROOM_SDK_LOCKED&&requests==0);
    CHECK(workers[1].quarantined);
    setup(2);CHECK(room_http_close(&cases[2],3,200)==ROOM_SDK_WAIT&&requests==1);
    result=ROOM_SDK_DRAINED;ms=200;
    CHECK(room_http_close(&cases[2],3,200)==ROOM_SDK_LOCKED&&polls==1);
    CHECK(room_http_close(&cases[2],3,300)==ROOM_SDK_LOCKED&&requests==1);
    setup(3);reject=true;CHECK(room_http_close(&cases[3],4,200)==ROOM_SDK_LOCKED&&requests==1);
    CHECK(room_http_close(&cases[3],4,200)==ROOM_SDK_LOCKED&&requests==1);
    setup(4);result=ROOM_SDK_LOCKED;CHECK(room_http_close(&cases[4],5,200)==ROOM_SDK_LOCKED);
    CHECK(workers[4].quarantined);CHECK(cases[4].handle==NULL);CHECK(room_http_close(&cases[4],5,200)==ROOM_SDK_LOCKED&&requests==1);
    setup(5);task=(TaskHandle_t)(void*)&other_token;
    CHECK(room_http_close(&cases[5],6,200)==ROOM_SDK_LOCKED&&requests==0);
    task=workers[5].owner;CHECK(room_http_close(&cases[5],7,200)==ROOM_SDK_LOCKED&&requests==0);
    CHECK(room_http_close(&cases[5],6,200)==ROOM_SDK_WAIT);ms=99;
    CHECK(room_http_close(&cases[5],6,200)==ROOM_SDK_LOCKED);
    setup(6);CHECK(room_http_close(&cases[6],7,200)==ROOM_SDK_WAIT);
    CHECK(room_http_close(&cases[6],7,201)==ROOM_SDK_LOCKED&&requests==1);
    printf("HTTP_LIFECYCLE_PASS checks=%u\n",checks);return 0;
}
