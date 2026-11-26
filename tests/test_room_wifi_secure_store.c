#include "room_wifi_secure_store.h"
#include "mock.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
static int scenario, registered, derived, initialized, opens, writes, commits, closes;
static uint8_t record[98]; static bool present;
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
esp_err_t nvs_open_from_partition(const char *p,const char *n,int mode,nvs_handle_t *h){assert(initialized==1&&!strcmp(p,"room_wifi")&&!strcmp(n,"network"));opens++;*h=42;if(mode==NVS_READONLY&&!present)return ESP_ERR_NVS_NOT_FOUND;return ESP_OK;}
esp_err_t nvs_get_blob(nvs_handle_t h,const char *k,void *v,size_t *l){assert(h==42&&!strcmp(k,"profile")&&*l==98);memcpy(v,record,98);return 0;}
esp_err_t nvs_set_blob(nvs_handle_t h,const char *k,const void *v,size_t l){assert(h==42&&!strcmp(k,"profile")&&l==98);writes++;memcpy(record,v,l);present=true;return 0;}
esp_err_t nvs_commit(nvs_handle_t h){assert(h==42);commits++;return scenario==8?2:0;}
void nvs_close(nvs_handle_t h){assert(h==42);closes++;}
int main(int argc,char **argv){assert(argc==2);scenario=atoi(argv[1]);room_enroll_store s=room_secure_store();room_enroll_profile p={.ssid_len=1,.pass_len=8,.ssid={'x'},.pass={'a','b','c','d','e','f','g','h'}};
assert(!s.ready(s.ctx));assert(!s.write(s.ctx,&p));assert(opens==0);
if(scenario>=1&&scenario<=6){assert(!room_secure_init(2));assert(!room_secure_init(2));assert(!s.ready(s.ctx));assert(room_secure_load(&p)==ROOM_SEC_UNAVAILABLE);assert(!opens&&!writes);assert(registered==(scenario>=4));assert(derived==(scenario>=5));assert(initialized==(scenario>=6));}
else {assert(room_secure_init(2));assert(!room_secure_init(2));assert(s.ready(s.ctx));room_enroll_profile out;assert(room_secure_load(&out)==ROOM_SEC_MISSING);assert(!out.ssid_len&&!out.pass_len);assert(s.write(s.ctx,&p)==(scenario!=8));assert(writes==1&&commits==1&&closes==1);if(scenario==8){assert(!s.ready(s.ctx));assert(!s.write(s.ctx,&p));}else{assert(room_secure_load(&out)==ROOM_SEC_VALID);assert(!memcmp(&p,&out,sizeof(p)));record[0]=9;assert(room_secure_load(&out)==ROOM_SEC_ERROR);assert(!out.ssid_len&&!out.pass_len);assert(!s.ready(s.ctx));assert(!s.write(s.ctx,&p));}}
printf("secure storage case %d PASS\n",scenario);return 0;}
