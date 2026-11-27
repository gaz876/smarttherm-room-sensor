#include "room_wifi_secure_store.h"
#include "mock.h"
#include "room_wifi_identity_provider.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
static int scenario, registered, derived, initialized, opens, writes, commits, closes;
static uint8_t record[98]; static bool present;
static uint8_t id_record[ROOM_ID_RECORD_SIZE];
static bool id_present,id_short,id_error;
static esp_partition_t part={false,false,0x6000}; static nvs_sec_scheme_t scheme;
#define F(n) const esp_efuse_desc_t *ESP_EFUSE_RD_DIS_KEY##n[]={NULL}; const esp_efuse_desc_t *ESP_EFUSE_WR_DIS_KEY##n[]={NULL};
F(0) F(1) F(2) F(3) F(4) F(5)
#undef F
const esp_partition_t *esp_partition_find_first(int a,int b,const char *p){assert(a==1&&b==2&&!strcmp(p,"room_wifi"));return scenario==1?NULL:&part;}
int esp_efuse_get_key_purpose(esp_efuse_block_t b){assert(b==EFUSE_BLK_KEY0+2);return scenario==2?0:8;}
bool esp_efuse_read_field_bit(const esp_efuse_desc_t **p){assert(p==ESP_EFUSE_RD_DIS_KEY2||p==ESP_EFUSE_WR_DIS_KEY2);return scenario!=3;}
esp_err_t nvs_sec_provider_register_hmac(const nvs_sec_config_hmac_t *h,nvs_sec_scheme_t **s){assert(h->hmac_key_id==2);registered++;*s=&scheme;return scenario==4?2:0;}
esp_err_t nvs_flash_read_security_cfg_v2(nvs_sec_scheme_t *s,nvs_sec_cfg_t *c){assert(s==&scheme);derived++;memset(c,9,sizeof(*c));return scenario==5?2:0;}
esp_err_t nvs_flash_secure_init_partition(const char *p,nvs_sec_cfg_t *c){assert(!strcmp(p,"room_wifi")&&c&&c->bytes[0]==9);initialized++;return scenario==6?2:0;}
esp_err_t nvs_open_from_partition(const char *p,const char *n,int mode,nvs_handle_t *h){assert(initialized==1&&!strcmp(p,"room_wifi"));opens++;
if(!strcmp(n,"setup_id")){assert(mode==NVS_READONLY);*h=43;return id_error?2:id_present?ESP_OK:ESP_ERR_NVS_NOT_FOUND;}
assert(!strcmp(n,"network"));*h=42;if(mode==NVS_READONLY&&!present)return ESP_ERR_NVS_NOT_FOUND;return ESP_OK;}
esp_err_t nvs_get_blob(nvs_handle_t h,const char *k,void *v,size_t *l){if(h==43){assert(!strcmp(k,"identity")&&*l==ROOM_ID_RECORD_SIZE);memcpy(v,id_record,sizeof(id_record));if(id_short)--*l;return ESP_OK;}
assert(h==42&&!strcmp(k,"profile")&&*l==98);memcpy(v,record,98);return 0;}
esp_err_t nvs_set_blob(nvs_handle_t h,const char *k,const void *v,size_t l){assert(h==42&&!strcmp(k,"profile")&&l==98);writes++;memcpy(record,v,l);present=true;return 0;}
esp_err_t nvs_commit(nvs_handle_t h){assert(h==42);commits++;return scenario==8?2:0;}
void nvs_close(nvs_handle_t h){assert(h==42||h==43);closes++;}
static int task_one,task_two;
static TaskHandle_t current=&task_one;
TaskHandle_t xTaskGetCurrentTaskHandle(void){return current;}
static void identity_record(void){
    memset(id_record,0,sizeof(id_record));id_record[0]=1;id_record[1]=4;id_record[2]=16;id_record[3]=16;
    memcpy(id_record+4,"mock",4);memcpy(id_record+36,"test-only-key-16",16);
    id_record[99]=1;id_record[163]=2;id_present=true;
}
static void check_zero(const void *ptr,size_t n){const uint8_t *p=ptr;for(size_t i=0;i<n;i++)assert(!p[i]);}
static int identity_case(void){
    room_setup_identity out;
    memset(&out,1,sizeof(out));assert(room_secure_identity_load(&out)==ROOM_SEC_UNAVAILABLE);
    check_zero(&out,sizeof(out));assert(!opens);
    assert(room_secure_init(2));identity_record();
    if(scenario==11)id_present=false;
    if(scenario==12)id_short=true;
    if(scenario==13)id_record[0]=2;
    if(scenario==14)id_record[4+5]=1;
    if(scenario==15)memset(id_record+99,0,64);
    if(scenario==16)memset(id_record+163,0,384);
    if(scenario==17)id_error=true;
    if(scenario==18)id_record[2]=15;
    if(scenario==19)id_record[3]=65;
    room_wifi_apsta_provider provider;
    bool ok=room_wifi_identity_provider_init(&provider);
    assert(ok==(scenario==10||scenario==20));
    assert(!writes&&!commits);
    if(!ok){assert(!provider.ready&&!provider.prepare);assert(!room_wifi_identity_provider_init(&provider));return 0;}
    assert(provider.ready(provider.ctx));
    room_wifi_apsta_window window;
    memset(&window,1,sizeof(window));
    current=&task_two;assert(!provider.ready(provider.ctx)&&!provider.prepare(provider.ctx,&window));check_zero(&window,sizeof(window));current=&task_one;
    assert(provider.prepare(provider.ctx,&window)&&window.sdk_id==1&&window.key_len==16&&window.salt_len==16&&window.verifier_len==384&&window.salt[0]==1&&window.verifier[0]==2);
    if(scenario==20){
        id_short=true;assert(!provider.prepare(provider.ctx,&window));check_zero(&window,sizeof(window));
        id_short=false;assert(!provider.ready(provider.ctx)&&!provider.prepare(provider.ctx,&window));check_zero(&window,sizeof(window));
    }else assert(provider.prepare(provider.ctx,&window)&&window.sdk_id==2);
    assert(!writes&&!commits);puts("identity provider mock PASS");return 0;
}
int main(int argc,char **argv){assert(argc==2);scenario=atoi(argv[1]);if(scenario>=10)return identity_case();room_enroll_store s=room_secure_store();room_enroll_profile p={.ssid_len=1,.pass_len=8,.ssid={'x'},.pass={'a','b','c','d','e','f','g','h'}};
assert(!s.ready(s.ctx));assert(!s.write(s.ctx,&p));assert(opens==0);
if(scenario>=1&&scenario<=6){assert(!room_secure_init(2));assert(!room_secure_init(2));assert(!s.ready(s.ctx));assert(room_secure_load(&p)==ROOM_SEC_UNAVAILABLE);assert(!opens&&!writes);assert(registered==(scenario>=4));assert(derived==(scenario>=5));assert(initialized==(scenario>=6));}
else {assert(room_secure_init(2));assert(!room_secure_init(2));assert(s.ready(s.ctx));room_enroll_profile out;assert(room_secure_load(&out)==ROOM_SEC_MISSING);assert(!out.ssid_len&&!out.pass_len);assert(s.write(s.ctx,&p)==(scenario!=8));assert(writes==1&&commits==1&&closes==1);if(scenario==8){assert(!s.ready(s.ctx));assert(!s.write(s.ctx,&p));}else{assert(room_secure_load(&out)==ROOM_SEC_VALID);assert(!memcmp(&p,&out,sizeof(p)));record[0]=9;assert(room_secure_load(&out)==ROOM_SEC_ERROR);assert(!out.ssid_len&&!out.pass_len);assert(!s.ready(s.ctx));assert(!s.write(s.ctx,&p));}}
printf("secure storage case %d PASS\n",scenario);return 0;}
