#include "room_wifi_owner.h"
#include <assert.h>
#include <stdio.h>
typedef struct { unsigned active,sta_starts,setup_starts,stop_calls; bool allow_stop,fail_setup; } radio;
static bool stop(void *v) { radio *r=v; ++r->stop_calls; if(!r->allow_stop)return false; r->active=0; return true; }
static bool station(void *v,uint64_t token) { radio *r=v; assert(!r->active && token); r->active=1; ++r->sta_starts; return true; }
static bool setup(void *v,uint64_t token) { radio *r=v; assert(!r->active && token); r->active=2; ++r->setup_starts; return !r->fail_setup; }
static void tick(room_wifi_owner *o,uint64_t t,bool manual,bool ready) { wifi_owner_step(o,t,manual,ready,(room_wifi_event){0}); }
static void online(room_wifi_owner *o,radio *r) { *r=(radio){0};wifi_owner_init(o,(room_wifi_transport){r,stop,station,setup},ROOM_WIFI_PROFILE_VALID,0,true);tick(o,0,false,true);wifi_owner_step(o,1,false,true,(room_wifi_event){WIFI_EVENT_IP,o->active});assert(wifi_owner_online(o)); }
int main(void) {
 radio r;room_wifi_owner o;online(&o,&r);
 tick(&o,10,true,true);assert(r.active==1&&!r.setup_starts&&o.pending_setup&&!o.active);
 tick(&o,1010,true,true);assert(o.pending_setup&&o.policy.deadline==300010&&!r.setup_starts);
 r.allow_stop=true;tick(&o,2010,false,true);assert(r.active==2&&r.sta_starts==1&&r.setup_starts==1&&!o.pending_setup&&o.active==2);
 wifi_owner_step(&o,2011,false,true,(room_wifi_event){WIFI_EVENT_IP,1});assert(o.policy.state==ROOM_WIFI_SETUP);
 wifi_owner_step(&o,2012,false,true,(room_wifi_event){WIFI_EVENT_SETUP_SAVED,2});assert(!r.active);tick(&o,2013,false,true);assert(r.active==1&&r.sta_starts==2);
 online(&o,&r);wifi_owner_step(&o,5,false,true,(room_wifi_event){WIFI_EVENT_LOST,1});assert(o.dirty&&!o.active);
 tick(&o,6,true,true);assert(o.pending_setup&&!r.setup_starts);r.allow_stop=true;tick(&o,1005,false,true);assert(r.active==2&&r.setup_starts==1);
 online(&o,&r);tick(&o,10,true,true);tick(&o,1010,false,false);assert(!o.pending_setup&&!r.setup_starts);r.allow_stop=true;tick(&o,2010,false,true);assert(!r.setup_starts&&r.active==1);
 online(&o,&r);tick(&o,10,true,true);tick(&o,300010,false,true);assert(!o.pending_setup&&!r.setup_starts);r.allow_stop=true;tick(&o,301010,false,true);assert(!r.setup_starts&&r.active==1);
 online(&o,&r);tick(&o,10,true,true);r.allow_stop=true;r.fail_setup=true;tick(&o,1010,false,true);assert(r.setup_starts==1&&!r.active&&!o.pending_setup);tick(&o,2010,false,true);assert(r.active==1&&r.setup_starts==1);
 online(&o,&r);tick(&o,10,true,false);assert(r.active==1&&!o.pending_setup&&!r.setup_starts);
 online(&o,&r);tick(&o,10,true,true);o.generation=UINT64_MAX;r.allow_stop=true;tick(&o,1010,false,true);assert(o.exhausted&&!r.active&&!r.setup_starts&&!o.pending_setup);
 puts("PASS: deferred BOOT, existing cleanup, expiry, revoked readiness, failed setup, return to STA, exclusivity, generation exhaustion");
}
