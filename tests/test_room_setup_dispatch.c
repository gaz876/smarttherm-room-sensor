#include "room_setup_dispatch.h"
#include <assert.h>
#include <stdio.h>

typedef struct { room_enroll_profile saved; unsigned writes, probes; bool ready, ok, link; uint64_t trial; } mock;
static bool ready(void *v) { return ((mock *)v)->ready; }
static bool write_record(void *v,const room_enroll_profile *p) { mock *m=v; m->writes++; m->saved=*p; return m->ok; }
static bool read_record(void *v,room_enroll_profile *p) { *p=((mock *)v)->saved; return true; }
static bool live(void *v,uint64_t t) { mock *m=v; m->probes++; return m->link && t==m->trial; }
static uint64_t token(const uint8_t *p) { uint64_t t=0; for (size_t i=3;i<11;i++) t=(t<<8)|p[i]; return t; }
static void zero(const void *v,size_t n) { const uint8_t *p=v; for (size_t i=0;i<n;i++) assert(!p[i]); }
static const uint8_t status[]={1,ROOM_SETUP_STATUS}, cancel[]={1,ROOM_SETUP_CANCEL};
static const uint8_t stage[]={1,ROOM_SETUP_STAGE,1,8,'x','a','b','c','d','e','f','g','h'};
static const uint8_t save1[]={1,ROOM_SETUP_SAVE,0,0,0,0,0,0,0,1},save2[]={1,ROOM_SETUP_SAVE,0,0,0,0,0,0,0,2};
int main(void) {
    mock m={.ready=true,.ok=true,.link=true}; room_enroll_store s={&m,ready,write_record,read_record};
    room_phone_setup p={0}; room_setup_dispatch d={0}; uint8_t out[ROOM_SETUP_REPLY_SIZE];
#define CALL(time,session,msg) room_setup_handle(&d,&p,time,session,msg,sizeof(msg),out,&s,live,&m)
#define EXPECT(time,session,msg,code,state,id) do { assert(CALL(time,session,msg)); assert(out[0]==1 && out[1]==code && out[2]==state && token(out)==id); } while (0)
    assert(phone_open(&p,0,PHONE_PROFILE_MISSING,false,true,&s));
    assert(!CALL(0,1,stage)); zero(out,sizeof(out)); assert(phone_claim(&p,1,1));
    assert(!CALL(2,2,stage)); zero(out,sizeof(out));
    EXPECT(2,1,stage,ROOM_REPLY_OK,1,1);
    EXPECT(3,1,save1,ROOM_REPLY_WAIT,1,1);
    EXPECT(102,1,save1,ROOM_REPLY_REJECTED,1,1); assert(!m.probes&&!m.writes);
    assert(phone_trial_result(&p,103,1,true));
    EXPECT(202,1,status,ROOM_REPLY_OK,2,1);
    EXPECT(302,1,save2,ROOM_REPLY_REJECTED,2,1); assert(!m.probes&&!m.writes);
    m.trial=1; m.link=false;
    EXPECT(402,1,save1,ROOM_REPLY_REJECTED,3,0); assert(m.probes==1&&!m.writes);
    zero(&p.enrollment.candidate,sizeof(p.enrollment.candidate));
    EXPECT(502,1,stage,ROOM_REPLY_REJECTED,3,0); phone_drained(&p);
    EXPECT(602,1,stage,ROOM_REPLY_OK,1,2); assert(phone_trial_result(&p,603,2,true));
    m.trial=2; m.link=true;
    EXPECT(702,1,save2,ROOM_REPLY_SAVED,3,0); assert(m.writes==1&&m.probes==2&&!p.window);
    assert(!CALL(802,1,save2)); zero(out,sizeof(out)); assert(m.writes==1); phone_drained(&p);
    assert(phone_open(&p,900,PHONE_PROFILE_VALID,true,true,&s)); assert(phone_claim(&p,901,2));
    EXPECT(902,2,stage,ROOM_REPLY_OK,1,3);
    EXPECT(903,2,cancel,ROOM_REPLY_CANCELLED,3,0); assert(!p.window&&m.writes==1);
    zero(&p.enrollment.candidate,sizeof(p.enrollment.candidate)); phone_drained(&p);
    assert(phone_open(&p,1000,PHONE_PROFILE_VALID,true,true,&s)); assert(phone_claim(&p,1001,3));
    EXPECT(1002,3,stage,ROOM_REPLY_OK,1,4);
    EXPECT(31002,3,status,ROOM_REPLY_OK,3,0); phone_drained(&p);
    const uint8_t malformed[]={0,99}; EXPECT(31102,3,malformed,ROOM_REPLY_REJECTED,0,0);
    EXPECT(31103,3,status,ROOM_REPLY_WAIT,0,0);
    assert(!CALL(31101,3,status)); assert(!p.window); zero(out,sizeof(out));
    assert(phone_open(&p,32000,PHONE_PROFILE_VALID,true,true,&s)); assert(phone_claim(&p,32001,4));
    EXPECT(32002,4,stage,ROOM_REPLY_OK,1,5); assert(phone_trial_result(&p,32003,5,true));
    const uint8_t save5[]={1,ROOM_SETUP_SAVE,0,0,0,0,0,0,0,5}; m.trial=5; m.ok=false;
    EXPECT(32102,4,save5,ROOM_REPLY_UNCERTAIN,3,0); assert(m.writes==2&&p.enrollment.storage_uncertain&&!p.window);
    phone_drained(&p); assert(!phone_open(&p,32200,PHONE_PROFILE_VALID,true,true,&s));
    assert(!CALL(32201,4,status)); zero(out,sizeof(out));
    puts("room setup dispatch PASS"); return 0;
}
