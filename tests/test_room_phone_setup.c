#include "room_phone_setup.h"
#include <assert.h>
#include <stdio.h>
typedef struct { bool available, ok; unsigned writes; room_enroll_profile saved; } fake_store;
static bool ready(void *p) { return ((fake_store *)p)->available; }
static bool save(void *p,const room_enroll_profile *v) { fake_store *s=p; s->writes++; s->saved=*v; return s->ok; }
static bool load(void *p,room_enroll_profile *v) { *v=((fake_store *)p)->saved; return true; }
static uint64_t stage(room_phone_setup *p,uint64_t now,uint64_t session,const room_enroll_store *s) {
    const uint8_t name[]={'x'}, pass[]={'a','b','c','d','e','f','g','h'};
    return phone_stage(p,now,session,s,name,sizeof(name),pass,sizeof(pass));
}
int main(void) {
    fake_store f={.available=true,.ok=true}; room_enroll_store s={&f,ready,save,load}; room_phone_setup p={0};
    assert(!phone_open(&p,0,PHONE_PROFILE_VALID,false,true,&s));
    assert(!phone_open(&p,0,PHONE_PROFILE_ERROR,true,true,&s));
    assert(!phone_open(&p,0,PHONE_PROFILE_MISSING,false,false,&s));
    f.available=false; assert(!phone_open(&p,0,PHONE_PROFILE_MISSING,false,true,&s)); f.available=true;
    assert(phone_open(&p,0,PHONE_PROFILE_MISSING,false,true,&s));
    assert(!stage(&p,1,1,&s)); assert(!phone_claim(&p,1,0)); assert(phone_claim(&p,1,1)); assert(!phone_claim(&p,1,2));
    assert(!stage(&p,2,2,&s)); uint64_t t=stage(&p,2,1,&s); assert(t==1);
    assert(phone_save(&p,3,1,t,true,&s)==ENROLL_REJECTED);
    assert(!phone_trial_result(&p,4,t+1,true)); assert(phone_trial_result(&p,4,t,true));
    assert(phone_save(&p,5,2,t,true,&s)==ENROLL_REJECTED); assert(f.writes==0);
    assert(!phone_trial_result(&p,6,t,false)); assert(p.drain); assert(!stage(&p,7,1,&s));
    phone_drained(&p); t=stage(&p,8,1,&s); assert(t==2); assert(phone_trial_result(&p,9,t,true));
    assert(phone_save(&p,10,1,t,false,&s)==ENROLL_REJECTED); assert(p.drain); assert(f.writes==0);
    phone_drained(&p); t=stage(&p,11,1,&s); assert(t==3); assert(phone_trial_result(&p,12,t,true));
    assert(phone_save(&p,13,1,t,true,&s)==ENROLL_SAVED); assert(!p.window&&p.drain&&f.writes==1);
    assert(!phone_open(&p,14,PHONE_PROFILE_VALID,true,true,&s)); phone_drained(&p);
    assert(!phone_open(&p,15,PHONE_PROFILE_MISSING,false,true,&s));
    assert(phone_open(&p,16,PHONE_PROFILE_VALID,true,true,&s)); assert(!phone_claim(&p,17,1)); assert(phone_claim(&p,17,2));
    t=stage(&p,18,2,&s); assert(t==4); phone_session_lost(&p,1); assert(p.window); phone_session_lost(&p,2); assert(!p.window&&p.drain);
    phone_drained(&p); assert(phone_open(&p,19,PHONE_PROFILE_VALID,true,true,&s)); assert(phone_claim(&p,20,3));
    t=stage(&p,21,3,&s); assert(t==5); phone_tick(&p,30021); assert(p.window&&p.drain); assert(!phone_trial_result(&p,30022,t,true));
    phone_drained(&p); phone_tick(&p,300019); assert(!p.window);
    assert(phone_open(&p,400000,PHONE_PROFILE_VALID,true,true,&s)); assert(phone_claim(&p,400001,4));
    t=stage(&p,400002,4,&s); assert(phone_trial_result(&p,400003,t,true)); f.ok=false;
    assert(phone_save(&p,400004,4,t,true,&s)==ENROLL_STORAGE_UNCERTAIN); assert(!p.window&&p.drain); phone_drained(&p);
    assert(!phone_open(&p,400005,PHONE_PROFILE_VALID,true,true,&s)); assert(f.writes==2);
    puts("phone setup owner PASS"); return 0;
}
