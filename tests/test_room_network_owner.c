#include "room_network_owner.h"
#include "room_network.h"
#include "freertos/task.h"
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>

/* Synthetic fixture: the actual new owner adapter and normal startup task
 * are linked. Lower-level storage/radio/transport APIs are inert doubles. */
static room_secure_status storage=ROOM_SEC_VALID;
static bool init_ok=true,identity_ok=true,station_ok=true,bind_ok=true,missing_ok=true,attach_ok=true;
static char calls[32]; static size_t used;
static int token;
static jmp_buf end;
static bool task_ok=true;
static void (*task_entry)(void *);
static unsigned delay_count;
static void mark(char c){assert(used+1<sizeof(calls)); calls[used++]=c;calls[used]=0;}
static void clear(void){used=0;calls[0]=0;storage=ROOM_SEC_VALID;init_ok=identity_ok=station_ok=bind_ok=missing_ok=attach_ok=true;}
static void expect(const char *s){if(strcmp(calls,s)){fprintf(stderr,"expected %s, got %s\n",s,calls);assert(0);}}
bool room_secure_init(unsigned key){assert(key==0);mark('I');return init_ok;}
room_secure_status room_secure_load(room_enroll_profile *p){mark('L');*p=(room_enroll_profile){.ssid_len=1,.pass_len=8};return storage;}
void enroll_wipe(void *p,size_t n){mark('W');memset(p,0,n);}
static bool provider_ready(void *v){assert(v==&token);return true;}
static bool provider_prepare(void *v,room_wifi_apsta_window *p){(void)v;(void)p;assert(0&&"startup must not open window");return false;}
bool room_wifi_identity_provider_init(room_wifi_apsta_provider *p){mark('P');if(!identity_ok)return false;*p=(room_wifi_apsta_provider){&token,provider_ready,provider_prepare};return true;}
bool room_wifi_idf_init(const uint8_t *s,size_t n,const uint8_t *p,size_t m){assert(s&&p&&n==1&&m==8);mark('S');return station_ok;}
bool room_wifi_apsta_link_bind(room_wifi_apsta_provider p){assert(p.ctx==&token&&p.ready==provider_ready&&p.prepare==provider_prepare);mark('B');return bind_ok;}
bool room_wifi_apsta_link_init_missing(room_wifi_apsta_provider p){assert(p.ctx==&token&&p.ready==provider_ready&&p.prepare==provider_prepare);mark('M');return missing_ok;}
static bool open_cb(void *c,const esp_netif_t *ap,uint64_t w){(void)c;(void)ap;(void)w;assert(0);return false;}
static room_phone_io_result recv_cb(void *c,bool *h,uint8_t *p,size_t n,size_t *m){(void)c;(void)h;(void)p;(void)n;(void)m;assert(0);return ROOM_PHONE_IO_ERROR;}
static room_phone_io_result send_cb(void *c,const uint8_t *p,size_t n,size_t *m){(void)c;(void)p;(void)n;(void)m;assert(0);return ROOM_PHONE_IO_ERROR;}
static bool close_cb(void *c){(void)c;assert(0);return false;}
static bool close_until(void *c,uint64_t w,uint64_t d){(void)c;(void)w;(void)d;assert(0);return false;}
bool room_wifi_apsta_link_attach_io_until(room_phone_io io,room_phone_timed_close close){assert(io.ctx==&token&&io.open==open_cb&&io.receive==recv_cb&&io.send==send_cb&&io.close==close_cb&&close==close_until);mark('A');return attach_ok;}
bool room_wifi_idf_step(bool *b){assert(!b);mark('T');return true;}
void mock_log(const char *t,const char *fmt,...){assert(t&&fmt);}
int xTaskCreate(void (*f)(void *),const char *name,unsigned stack,void *arg,unsigned priority,void *out){assert(f&&!strcmp(name,"room_net")&&stack==6144&&!arg&&priority==4&&!out);mark('C');if(!task_ok)return 0;task_entry=f;return pdPASS;}
void vTaskDelete(void *p){assert(!p);mark('D');longjmp(end,1);}
void vTaskDelay(TickType_t t){assert(t==5);if(++delay_count==3)longjmp(end,1);}
static room_network_setup_backend backend(void){return (room_network_setup_backend){{&token,open_cb,recv_cb,send_cb,close_cb},close_until};}
static void check(unsigned key,const room_network_setup_backend *b,bool expected,room_secure_status want,const char *trace){room_secure_status result=ROOM_SEC_ERROR;assert(room_network_owner_prepare(key,b,&result)==expected);assert(result==want);expect(trace);}
static void owner_cases(void){
    room_network_setup_backend b=backend();
    clear();check(6,&b,false,ROOM_SEC_UNAVAILABLE,"");
    clear();b.close_until=NULL;check(0,&b,false,ROOM_SEC_UNAVAILABLE,"");b=backend();
    clear();b.io.open=NULL;check(0,&b,false,ROOM_SEC_UNAVAILABLE,"");b=backend();
    clear();b.io.receive=NULL;check(0,&b,false,ROOM_SEC_UNAVAILABLE,"");b=backend();
    clear();b.io.send=NULL;check(0,&b,false,ROOM_SEC_UNAVAILABLE,"");b=backend();
    clear();b.io.close=NULL;check(0,&b,false,ROOM_SEC_UNAVAILABLE,"");b=backend();
    clear();init_ok=false;check(0,&b,false,ROOM_SEC_UNAVAILABLE,"I");
    clear();storage=ROOM_SEC_UNAVAILABLE;check(0,&b,false,ROOM_SEC_UNAVAILABLE,"ILW");
    clear();storage=ROOM_SEC_ERROR;check(0,&b,false,ROOM_SEC_ERROR,"ILW");
    clear();storage=ROOM_SEC_MISSING;check(0,NULL,false,ROOM_SEC_MISSING,"ILW");
    clear();check(0,NULL,true,ROOM_SEC_VALID,"ILSW");
    clear();station_ok=false;check(0,NULL,false,ROOM_SEC_VALID,"ILSW");
    clear();identity_ok=false;check(0,&b,false,ROOM_SEC_VALID,"ILPW");
    clear();station_ok=false;check(0,&b,false,ROOM_SEC_VALID,"ILPSW");
    clear();bind_ok=false;check(0,&b,false,ROOM_SEC_VALID,"ILPSBW");
    clear();attach_ok=false;check(0,&b,false,ROOM_SEC_VALID,"ILPSBAW");
    clear();check(0,&b,true,ROOM_SEC_VALID,"ILPSBAW");
    clear();storage=ROOM_SEC_MISSING;identity_ok=false;check(0,&b,false,ROOM_SEC_MISSING,"ILPW");
    clear();storage=ROOM_SEC_MISSING;missing_ok=false;check(0,&b,false,ROOM_SEC_MISSING,"ILPMW");
    clear();storage=ROOM_SEC_MISSING;attach_ok=false;check(0,&b,false,ROOM_SEC_MISSING,"ILPMAW");
    clear();storage=ROOM_SEC_MISSING;check(0,&b,true,ROOM_SEC_MISSING,"ILPMAW");
    clear();assert(room_network_owner_prepare(0,NULL,NULL));expect("ILSW");
    puts("PASS: 22 owner preparation cases, timed backend attachment order and no premature owner step");
}
int main(int argc,char **argv){
    assert(argc==2);
    if(!strcmp(argv[1],"owner")){owner_cases();return 0;}
    clear();assert(room_network_start(-1));assert(!room_network_start(-2));assert(!room_network_start(6));expect("");
    task_ok=false;assert(!room_network_start(0));expect("C");task_ok=true;assert(room_network_start(0));assert(!room_network_start(0));expect("CC");
    if(!strcmp(argv[1],"missing"))storage=ROOM_SEC_MISSING;
    else if(!strcmp(argv[1],"storage-error"))storage=ROOM_SEC_ERROR;
    else if(!strcmp(argv[1],"init-error"))init_ok=false;
    else if(!strcmp(argv[1],"station-error"))station_ok=false;
    else assert(!strcmp(argv[1],"valid"));
    assert(task_entry);if(!setjmp(end)){task_entry(NULL);assert(0);}
    if(!strcmp(argv[1],"valid"))expect("CCILSWTTT");
    else if(!strcmp(argv[1],"init-error"))expect("CCID");
    else if(!strcmp(argv[1],"station-error"))expect("CCILSWD");
    else expect("CCILWD");
    printf("PASS: normal startup %s, no setup backend activation\n",argv[1]);
}
