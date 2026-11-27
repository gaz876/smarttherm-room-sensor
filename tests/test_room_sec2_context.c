#include "room_sec2_context.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
struct protocomm { bool live; unsigned phase; };
static struct protocomm p;
const protocomm_security_t protocomm_security2={2};
static unsigned fail_at,news,deletes,opens,closes;
static int dummy;
static const protocomm_security2_params_t *borrowed;
static void identity_live(void){
    assert(borrowed&&borrowed->salt_len==16&&borrowed->verifier_len==384);
    assert(borrowed->salt[0]=='s'&&borrowed->verifier[0]=='v');
}
static void wiped(const room_sec2_context *c){
    assert(!c->identity.salt&&!c->identity.verifier&&!c->identity.salt_len&&!c->identity.verifier_len);
    for(size_t i=0;i<sizeof(c->salt);i++)assert(!c->salt[i]);
    for(size_t i=0;i<sizeof(c->verifier);i++)assert(!c->verifier[i]);
}
static esp_err_t endpoint(uint32_t s,const uint8_t *in,ssize_t n,uint8_t **out,ssize_t *m,void *ctx) {
    (void)s;(void)in;(void)n;(void)out;(void)m;(void)ctx; return ESP_FAIL;
}
protocomm_t *protocomm_new(void) { news++; if(fail_at==1)return NULL; assert(!p.live);p=(struct protocomm){true,1};return &p; }
void protocomm_delete(protocomm_t *q) { assert(q==&p&&p.live);if(borrowed)identity_live();borrowed=NULL;p.live=false;deletes++; }
esp_err_t protocomm_set_security(protocomm_t *q,const char *name,const protocomm_security_t *sec,const void *params) {
    assert(q==&p&&p.live&&p.phase==1&&sec==&protocomm_security2&&params&&!strcmp(name,"room-session"));
    borrowed=params;identity_live();
    if(fail_at==2)return ESP_FAIL;
    p.phase=2;return ESP_OK;
}
esp_err_t protocomm_add_endpoint(protocomm_t *q,const char *name,protocomm_req_handler_t h,void *ctx) {
    assert(q==&p&&p.live&&p.phase==2&&h==endpoint&&ctx==&dummy&&!strcmp(name,"room-command"));
    if(fail_at==3)return ESP_FAIL;
    p.phase=3;return ESP_OK;
}
esp_err_t protocomm_open_session(protocomm_t *q,uint32_t session) {
    assert(q==&p&&p.live&&p.phase==3&&session==7);opens++;identity_live();
    if(fail_at==4)return ESP_FAIL;
    p.phase=4;return ESP_OK;
}
esp_err_t protocomm_close_session(protocomm_t *q,uint32_t session) {
    assert(q==&p&&p.live&&p.phase==4&&session==7);closes++;identity_live();
    if(fail_at==5)return ESP_FAIL;
    p.phase=3;return ESP_OK;
}
int main(void) {
    char salt[16]={'s'},verifier[384]={'v'}; /* synthetic, not usable identity */
    protocomm_security2_params_t id={salt,sizeof(salt),verifier,sizeof(verifier)};
    room_sec2_context c={0};
    assert(!room_sec2_create(NULL,&id,endpoint,&dummy));
    assert(!room_sec2_create(&c,NULL,endpoint,&dummy));
    assert(!room_sec2_create(&c,&id,NULL,&dummy));
    id.verifier_len=383;assert(!room_sec2_create(&c,&id,endpoint,&dummy));id.verifier_len=384;
    id.salt_len=7;assert(!room_sec2_create(&c,&id,endpoint,&dummy));id.salt_len=16;
    assert(!news&&!deletes);
    for(fail_at=1;fail_at<=3;fail_at++) { assert(!room_sec2_create(&c,&id,endpoint,&dummy)); assert(!c.pc&&!p.live);wiped(&c); }
    assert(news==3&&deletes==2);
    fail_at=0;assert(room_sec2_create(&c,&id,endpoint,&dummy));
    assert(borrowed==&c.identity&&borrowed->salt==c.salt&&borrowed->verifier==c.verifier);
    memset(salt,0,sizeof(salt));memset(verifier,0,sizeof(verifier));identity_live();
    assert(!room_sec2_create(&c,&id,endpoint,&dummy));
    assert(!room_sec2_open(&c,0)&&!room_sec2_open(&c,UINT32_MAX));
    assert(room_sec2_open(&c,7)&&!room_sec2_open(&c,7));
    assert(!room_sec2_destroy(&c,false)&&!closes);
    assert(room_sec2_destroy(&c,true)&&closes==1&&!p.live&&!c.pc);
    wiped(&c);assert(room_sec2_destroy(&c,true));
    salt[0]='s';verifier[0]='v';
    assert(room_sec2_create(&c,&id,endpoint,&dummy));assert(room_sec2_open(&c,7));
    fail_at=5;assert(!room_sec2_destroy(&c,true)&&c.failed&&c.pc);
    assert(!room_sec2_destroy(&c,true)&&!room_sec2_open(&c,7)&&!room_sec2_create(&c,&id,endpoint,&dummy));
    assert(closes==2&&p.live);identity_live();p.live=false;borrowed=NULL; /* reset fake only, never on target */
    room_sec2_context d={0};fail_at=0;assert(room_sec2_create(&d,&id,endpoint,&dummy));
    fail_at=4;assert(!room_sec2_open(&d,7)&&d.failed);
    assert(!room_sec2_destroy(&d,true)&&!room_sec2_open(&d,7));assert(opens==3&&p.live);identity_live();
    puts("room sec2 context PASS (mock SDK)");return 0;
}
