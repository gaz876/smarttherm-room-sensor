#include "room_wifi_enrollment.h"
#include <assert.h>
#include <stdio.h>
typedef struct { bool ready,write_ok,read_ok,mismatch; unsigned writes,reads; room_enroll_profile saved; } mock;
static bool ready(void *p) { return ((mock *)p)->ready; }
static bool write_record(void *p,const room_enroll_profile *v) { mock *m=p; m->writes++; m->saved=*v; return m->write_ok; }
static bool read_record(void *p,room_enroll_profile *v) { mock *m=p; m->reads++; *v=m->saved; if (m->mismatch) v->ssid[0]^=1; return m->read_ok; }
static const uint8_t ssid[]={ 't','e','s','t' }, pass[]={ '0','1','2','3','4','5','6','7' };
static uint64_t stage(room_enroll *e,uint64_t now,uint64_t end,bool auth,const room_enroll_store *s) { return enroll_stage(e,now,end,auth,s,ssid,sizeof(ssid),pass,sizeof(pass)); }
static void empty(const room_enroll *e) { const unsigned char *p=(const unsigned char *)&e->candidate; for(size_t i=0;i<sizeof(e->candidate);i++) assert(p[i]==0); assert(e->state==ENROLL_IDLE); }
int main(void) {
    room_enroll e={0}; mock m={.ready=true,.write_ok=true,.read_ok=true}; room_enroll_store s={&m,ready,write_record,read_record};
    assert(!stage(&e,0,100000,false,&s)); m.ready=false; assert(!stage(&e,0,100000,true,&s)); empty(&e); m.ready=true;
    assert(!enroll_stage(&e,0,100000,true,&s,ssid,0,pass,sizeof(pass)));
    assert(!enroll_stage(&e,0,100000,true,&s,ssid,sizeof(ssid),pass,7));
    assert(!stage(&e,100,100,true,&s)); assert(m.writes==0);
    uint64_t t=stage(&e,0,100000,true,&s); assert(t==1); assert(!stage(&e,1,100000,true,&s));
    assert(enroll_commit(&e,1,t,true,true,&s)==ENROLL_REJECTED); assert(!enroll_observe(&e,2,t+1,true)); assert(m.writes==0);
    assert(enroll_observe(&e,3,t,true)); assert(enroll_commit(&e,4,t,false,true,&s)==ENROLL_REJECTED);
    assert(enroll_commit(&e,5,t,true,true,&s)==ENROLL_SAVED); empty(&e); assert(m.writes==1 && m.reads==1);
    assert(enroll_commit(&e,6,t,true,true,&s)==ENROLL_REJECTED);
    t=stage(&e,10,20,true,&s); assert(t==2); assert(!enroll_observe(&e,20,t,true)); empty(&e);
    t=stage(&e,30,100000,true,&s); assert(t==3); assert(!enroll_observe(&e,31,t-1,true)); assert(!enroll_observe(&e,32,t,false)); empty(&e);
    t=stage(&e,40,100000,true,&s); assert(enroll_observe(&e,41,t,true)); assert(enroll_commit(&e,42,t,true,false,&s)==ENROLL_REJECTED); empty(&e);
    t=stage(&e,50,100000,true,&s); assert(enroll_observe(&e,51,t,true)); m.ready=false; assert(enroll_commit(&e,52,t,true,true,&s)==ENROLL_REJECTED); empty(&e); m.ready=true; assert(m.writes==1);
    for (unsigned i=0;i<3;i++) {
        m.write_ok=i!=0; m.read_ok=i!=1; m.mismatch=i==2;
        t=stage(&e,100,100000,true,&s); assert(enroll_observe(&e,101,t,true)); assert(enroll_commit(&e,102,t,true,true,&s)==ENROLL_STORAGE_UNCERTAIN); empty(&e);
        assert(enroll_commit(&e,103,t,true,true,&s)==ENROLL_REJECTED);
        assert(e.storage_uncertain && !stage(&e,104,100000,true,&s));
        e=(room_enroll){0}; /* Independent mock instance; not a recovery procedure. */
    }
    assert(m.writes==4); t=stage(&e,100,100000,true,&s); assert(t); enroll_tick(&e,30100); empty(&e);
    e.generation=UINT64_MAX; assert(!stage(&e,0,100000,true,&s)); empty(&e);
    puts("enrollment transaction PASS"); return 0;
}
