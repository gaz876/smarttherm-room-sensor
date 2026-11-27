#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "room_http_async.h"

static room_http_lifecycle life[9];
static room_http_async async[9];
static room_lease callbacks[9];
static int server,other;
static unsigned begins,completes,checks,mode;
#define CHECK(x) do { ++checks; assert(x); } while(0)
static httpd_req_t original={&server};
esp_err_t httpd_req_async_handler_begin(httpd_req_t *r,httpd_req_t **out){
    ++begins;CHECK(r==&original);CHECK(*out==NULL);
    if(mode==1)return -1;
    if(mode==6)return ESP_ERR_NO_MEM;
    if(mode==2)return ESP_OK;
    if(mode==3){*out=r;return ESP_OK;}
    *out=malloc(sizeof **out);CHECK(*out);**out=*r;
    if(mode==4){free(*out);*out=NULL;return -1;}
    return ESP_OK;
}
esp_err_t httpd_req_async_handler_complete(httpd_req_t *r){
    ++completes;CHECK(r&&r!=&original);CHECK(r->handle==&server);
    free(r);return mode==5?-1:ESP_OK;
}
static void setup(unsigned i){
    mode=0;begins=completes=0;life[i].handle=&server;
    CHECK(room_admission_init(&life[i].gate,i+1));
    CHECK(room_http_enter(&life[i],i+1,&callbacks[i]));
}
static void success(unsigned i){
    setup(i);CHECK(room_http_async_begin(&async[i],&life[i],i+1,&callbacks[i],&original));
    CHECK(!room_http_async_begin(&async[i],&life[i],i+1,&callbacks[i],&original));
    CHECK(begins==1&&async[i].request&&async[i].request!=&original);
    CHECK(room_admission_close(&life[i].gate,i+1));
    CHECK(room_admission_leave(&callbacks[i]));
    CHECK(!room_admission_drained(&life[i].gate,i+1));
    CHECK(room_http_async_complete(&async[i]));
    CHECK(async[i].request==NULL&&async[i].state==ROOM_ASYNC_DONE&&completes==1);
    CHECK(!room_http_async_complete(&async[i]));
    CHECK(room_admission_drained(&life[i].gate,i+1));
}
static void failure(unsigned i,unsigned m){
    setup(i);mode=m;
    if(m==5){CHECK(room_http_async_begin(&async[i],&life[i],i+1,&callbacks[i],&original));CHECK(!room_http_async_complete(&async[i]));}
    else CHECK(!room_http_async_begin(&async[i],&life[i],i+1,&callbacks[i],&original));
    CHECK(async[i].state==ROOM_ASYNC_UNCERTAIN&&async[i].request==NULL&&begins==1);
    room_lease rejected={0};CHECK(!room_http_enter(&life[i],i+1,&rejected));
    CHECK(room_admission_leave(&callbacks[i]));CHECK(!room_admission_drained(&life[i].gate,i+1));
    CHECK(!room_http_async_complete(&async[i]));CHECK(completes==(m==5?1u:0u));
}
int main(void){
    success(0);
    for(unsigned m=1;m<=5;++m)failure(m,m);
    setup(6);httpd_req_t wrong={&other};room_lease unrelated={0};
    CHECK(!room_http_async_begin(&async[6],&life[6],7,&unrelated,&original));
    CHECK(!room_http_async_begin(&async[6],&life[6],7,&callbacks[6],&wrong));
    CHECK(!room_http_async_begin(&async[6],&life[6],8,&callbacks[6],&original));
    CHECK(begins==0&&!async[6].lease.gate);
    CHECK(room_admission_close(&life[6].gate,7));
    CHECK(!room_http_async_begin(&async[6],&life[6],7,&callbacks[6],&original));
    CHECK(room_admission_leave(&callbacks[6]));CHECK(room_admission_drained(&life[6].gate,7));
    success(7);
    setup(8);mode=6;
    CHECK(!room_http_async_begin(&async[8],&life[8],9,&callbacks[8],&original));
    CHECK(async[8].state==ROOM_ASYNC_DONE&&!async[8].lease.gate&&begins==1);
    CHECK(room_admission_close(&life[8].gate,9));CHECK(room_admission_leave(&callbacks[8]));
    CHECK(room_admission_drained(&life[8].gate,9));
    printf("HTTP_ASYNC_PASS checks=%u\n",checks);return 0;
}
