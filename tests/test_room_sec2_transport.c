#include "room_sec2_transport.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
struct protocomm { protocomm_req_handler_t handler; void *ctx; bool auth; };
const protocomm_security_t protocomm_security2={2};
protocomm_t *protocomm_new(void) { return calloc(1,sizeof(protocomm_t)); }
void protocomm_delete(protocomm_t *p) { free(p); }
esp_err_t protocomm_set_security(protocomm_t *p,const char *s,const protocomm_security_t *sec,const void *id) { assert(p&&s&&sec==&protocomm_security2&&id); return ESP_OK; }
esp_err_t protocomm_add_endpoint(protocomm_t *p,const char *s,protocomm_req_handler_t h,void *v) { assert(s); p->handler=h;p->ctx=v;return ESP_OK; }
esp_err_t protocomm_open_session(protocomm_t *p,uint32_t id) { assert(p&&id==7);return ESP_OK; }
esp_err_t protocomm_close_session(protocomm_t *p,uint32_t id) { assert(p&&id==7);return ESP_OK; }
/* Synthetic SDK only: no crypto and no real network. */
esp_err_t protocomm_req_handle(protocomm_t *p,const char *ep,uint32_t id,const uint8_t *in,ssize_t len,uint8_t **out,ssize_t *n) {
    *out=NULL;*n=0;
    if (!strcmp(ep,"room-session")) { p->auth=true;*out=calloc(1,1);*n=1;return *out?ESP_OK:ESP_FAIL; }
    if (!p->auth) return ESP_FAIL;
    uint8_t *plain=NULL;ssize_t m=0;
    esp_err_t rc=p->handler(id,in,len-16,&plain,&m,p->ctx);
    if (rc!=ESP_OK) return rc;
    assert(m==11);*out=calloc(1,27);if(*out)memcpy(*out,plain,11);free(plain);*n=27;return *out?ESP_OK:ESP_FAIL;
}
typedef struct { uint64_t now; unsigned writes; room_enroll_profile saved; } fake;
static uint64_t clock_ms(void *v){return ((fake *)v)->now;}
static bool ready(void *v){return v!=NULL;}
static bool write_record(void *v,const room_enroll_profile *p){fake*f=v;f->saved=*p;f->writes++;return true;}
static bool read_record(void *v,room_enroll_profile *p){*p=((fake*)v)->saved;return true;}
static bool live(void *v,uint64_t trial){return v&&trial;}
int main(void){
    room_sec2_transport t={0};room_setup_bridge b={0};fake f={0};room_enroll_store s={&f,ready,write_record,read_record};
    const char salt[16]={0},verifier[384]={0};protocomm_security2_params_t id={salt,16,verifier,384};
    const uint8_t *out;size_t n;uint8_t hs[]={1},stage[29]={1,ROOM_SETUP_STAGE,1,8,'x','a','b','c','d','e','f','g','h'},save[26]={1,ROOM_SETUP_SAVE,0,0,0,0,0,0,0,1};
    assert(bridge_open(&b,0,PHONE_PROFILE_MISSING,false,true,&s)==1);
    assert(room_sec2_transport_start(&t,&b,&id,7,1,&s,clock_ms,live,&f));
    assert(!room_sec2_transport_exchange(&t,false,stage,sizeof(stage),&out,&n));assert(!f.writes&&t.stopped);
    assert(!room_sec2_transport_destroy(&t,false));assert(room_sec2_transport_destroy(&t,true));
    f.now=100;assert(bridge_open(&b,100,PHONE_PROFILE_VALID,true,true,&s)==2);
    assert(room_sec2_transport_start(&t,&b,&id,7,2,&s,clock_ms,live,&f));
    assert(room_sec2_transport_exchange(&t,true,hs,sizeof(hs),&out,&n)&&n==1&&!b.phone.session);
    assert(!room_sec2_transport_exchange(&t,false,stage,sizeof(stage),&out,&n));assert(room_sec2_transport_finish(&t,true));
    f.now=101;assert(room_sec2_transport_exchange(&t,false,stage,sizeof(stage),&out,&n)&&n==27&&out[1]==ROOM_REPLY_OK);
    assert(room_sec2_transport_finish(&t,true));assert(bridge_trial(&b,102,2,1,true));
    f.now=201;assert(room_sec2_transport_exchange(&t,false,save,sizeof(save),&out,&n)&&out[1]==ROOM_REPLY_SAVED&&f.writes==1);
    assert(b.life.phase==SETUP_FINAL&&!t.stopped);assert(!room_sec2_transport_destroy(&t,false));
    assert(room_sec2_transport_finish(&t,true)&&t.stopped);assert(room_sec2_transport_destroy(&t,true));
    f.now=300;assert(bridge_open(&b,300,PHONE_PROFILE_VALID,true,true,&s)==3);assert(room_sec2_transport_start(&t,&b,&id,7,3,&s,clock_ms,live,&f));
    assert(room_sec2_transport_exchange(&t,true,hs,sizeof(hs),&out,&n));f.now=2300;assert(!room_sec2_transport_finish(&t,true)&&t.stopped);assert(room_sec2_transport_destroy(&t,true));
    puts("offline Security 2 transport mock PASS");return 0;
}
