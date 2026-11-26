#include "room_wifi_owner.h"
#include <assert.h>
#include <stdio.h>
typedef struct { unsigned starts,stops,setups; bool running,fail_start,fail_stop; uint64_t token; } mock;
static bool stop(void *p) { mock *m=p; ++m->stops; if(m->fail_stop)return false; m->running=false;return true; }
static bool station(void *p,uint64_t token) { mock *m=p; assert(!m->running);m->running=true;m->token=token;++m->starts;return !m->fail_start; }
static bool setup(void *p,uint64_t token) { mock *m=p;assert(!m->running);m->running=true;m->token=token;++m->setups;return !m->fail_start; }
static void tick(room_wifi_owner *o,uint64_t t) { wifi_owner_step(o,t,false,true,(room_wifi_event){0}); }
static void event(room_wifi_owner *o,uint64_t t,room_wifi_event_kind k,uint64_t token) { wifi_owner_step(o,t,false,true,(room_wifi_event){k,token}); }
int main(void) {
    mock m={0}; room_wifi_owner o; room_wifi_transport io={&m,stop,station,setup};
    wifi_owner_init(&o,io,ROOM_WIFI_PROFILE_VALID,0,true);tick(&o,0);assert(m.starts==1 && o.active==1);
    event(&o,1,WIFI_EVENT_IP,0);assert(!wifi_owner_online(&o));
    event(&o,30000,WIFI_EVENT_IP,1);assert(!wifi_owner_online(&o) && !m.running);
    tick(&o,31000);assert(o.active==2);event(&o,31001,WIFI_EVENT_IP,1);assert(!wifi_owner_online(&o));
    event(&o,31002,WIFI_EVENT_IP,2);assert(wifi_owner_online(&o));
    m.fail_stop=true;event(&o,31003,WIFI_EVENT_LOST,2);assert(!wifi_owner_online(&o) && m.running && o.dirty);
    tick(&o,32003);assert(m.starts==2 && m.running);m.fail_stop=false;tick(&o,33003);assert(m.starts==3 && o.active==3);
    m.fail_start=true;event(&o,33004,WIFI_EVENT_LOST,3);tick(&o,35004);assert(m.starts==4 && !m.running && o.active==0);
    m.fail_start=false;tick(&o,39004);assert(m.starts==5 && o.active==5);
    event(&o,39005,WIFI_EVENT_IP,5);assert(wifi_owner_online(&o));
    wifi_owner_step(&o,39006,true,false,(room_wifi_event){0});assert(wifi_owner_online(&o) && m.setups==0);
    wifi_owner_step(&o,39007,true,true,(room_wifi_event){WIFI_EVENT_IP,5});assert(m.setups==1 && o.active==6 && !wifi_owner_online(&o));
    event(&o,339007,WIFI_EVENT_SETUP_SAVED,6);assert(o.policy.profile==ROOM_WIFI_PROFILE_VALID && !m.running);
    tick(&o,339008);assert(m.starts==6 && o.active==7);
    event(&o,339009,WIFI_EVENT_LOST,7);assert(m.setups==1);
    m=(mock){0};wifi_owner_init(&o,io,ROOM_WIFI_PROFILE_MISSING,0,true);assert(m.setups==1 && o.active==1);
    event(&o,300000,WIFI_EVENT_SETUP_SAVED,1);assert(o.policy.profile==ROOM_WIFI_PROFILE_MISSING && o.policy.state==ROOM_WIFI_OFF && !m.running);
    wifi_owner_step(&o,300001,true,true,(room_wifi_event){0});assert(o.active==2);
    event(&o,300002,WIFI_EVENT_SETUP_SAVED,2);tick(&o,300003);assert(o.policy.profile==ROOM_WIFI_PROFILE_VALID && m.starts==1);
    event(&o,300004,WIFI_EVENT_LOST,3);o.generation=UINT64_MAX;tick(&o,301004);assert(o.exhausted && !m.running);
    room_boot_button b={0};assert(!wifi_boot_sample(&b,true,0));assert(!wifi_boot_sample(&b,true,4000));
    assert(!wifi_boot_sample(&b,false,4001));assert(!wifi_boot_sample(&b,true,4020));assert(!wifi_boot_sample(&b,false,4030));assert(!wifi_boot_sample(&b,false,4080));
    assert(!wifi_boot_sample(&b,true,4100));assert(!wifi_boot_sample(&b,true,4150));assert(!wifi_boot_sample(&b,true,7149));assert(wifi_boot_sample(&b,true,7150));assert(!wifi_boot_sample(&b,true,9000));
    assert(!wifi_boot_sample(&b,false,9001));assert(!wifi_boot_sample(&b,false,9051));assert(!wifi_boot_sample(&b,true,9100));assert(!wifi_boot_sample(&b,true,9150));assert(wifi_boot_sample(&b,true,12150));
    puts("PASS: transport generations, late events, cleanup retries, failed starts, setup gates, token exhaustion, BOOT debounce/hold");
}
