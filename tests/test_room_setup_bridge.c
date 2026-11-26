#include "room_setup_bridge.h"
#include <assert.h>
#include <stdio.h>
typedef struct { uint64_t now, live_delay, write_delay; unsigned writes, reads; bool ok, live; room_enroll_profile saved; } fake;
static uint64_t clock_ms(void *v) { return ((fake *)v)->now; }
static bool ready(void *v) { return ((fake *)v)->ok; }
static bool write_record(void *v,const room_enroll_profile *p) { fake *f=v; f->writes++; f->saved=*p; f->now+=f->write_delay; return true; }
static bool read_record(void *v,room_enroll_profile *p) { fake *f=v; f->reads++; *p=f->saved; return true; }
static bool live(void *v,uint64_t t) { fake *f=v; assert(t); f->now+=f->live_delay; return f->live; }
static void zero(const void *v,size_t n) { const uint8_t *p=v; for(size_t i=0;i<n;i++) assert(!p[i]); }
static const uint8_t stage[]={1,ROOM_SETUP_STAGE,1,8,'x','a','b','c','d','e','f','g','h'};
static const uint8_t status[]={1,ROOM_SETUP_STATUS},cancel[]={1,ROOM_SETUP_CANCEL};
static void save_msg(uint8_t p[10],uint64_t id) { p[0]=1;p[1]=ROOM_SETUP_SAVE; for(size_t i=10;i>2;) { p[--i]=(uint8_t)id;id>>=8; } }
int main(void) {
    fake f={.ok=true,.live=true}; room_enroll_store store={&f,ready,write_record,read_record};
    room_setup_bridge b={0}; uint8_t out[11],save[10];
#define REQ(w,s,msg) bridge_request(&b,w,s,msg,sizeof(msg),out,&store,clock_ms,live,&f)
    assert(bridge_open(&b,0,PHONE_PROFILE_MISSING,false,true,&store)==1);
    assert(!REQ(1,1,stage)); zero(out,sizeof(out));
    f.now=1; assert(bridge_claim(&b,1,1,1));
    uint64_t t=REQ(1,1,stage); assert(t==1&&out[1]==ROOM_REPLY_OK);
    assert(!REQ(1,1,status)); zero(out,sizeof(out));
    assert(!bridge_trial(&b,2,1,1,true));
    assert(bridge_sent(&b,3,1,1,t,true)); assert(bridge_trial(&b,4,1,1,true));
    f.now=101; save_msg(save,1); t=REQ(1,1,save); assert(t==2&&out[1]==ROOM_REPLY_SAVED&&f.writes==1);
    assert(!b.phone.window&&b.life.phase==SETUP_FINAL&&!b.stopping);
    assert(!bridge_drained(&b,102,1)); assert(!bridge_open(&b,103,PHONE_PROFILE_VALID,true,true,&store));
    assert(!bridge_sent(&b,104,1,1,t-1,true)); assert(bridge_sent(&b,105,1,1,t,true)&&b.stopping);
    assert(!bridge_drained(&b,106,2)); assert(bridge_drained(&b,107,1));
    assert(bridge_open(&b,200,PHONE_PROFILE_VALID,true,true,&store)==2);
    assert(!bridge_claim(&b,201,1,2)); assert(bridge_claim(&b,202,2,2));
    f.now=203;t=REQ(2,2,stage); assert(t); assert(bridge_sent(&b,204,2,2,t,true));
    assert(!bridge_trial(&b,205,1,2,true)); assert(!bridge_trial(&b,206,2,1,true));
    assert(!bridge_trial(&b,207,2,2,false)&&b.phone.drain);
    assert(!bridge_trial_drained(&b,208,2,1)); assert(bridge_trial_drained(&b,209,2,2));
    f.now=303;t=REQ(2,2,stage); assert(t&&out[1]==ROOM_REPLY_OK); assert(bridge_sent(&b,304,2,2,t,true));
    assert(bridge_trial(&b,305,2,3,true));
    f.now=403; f.live_delay=2000; save_msg(save,3); assert(!REQ(2,2,save));
    assert(b.stopping&&f.writes==1);zero(out,sizeof(out));zero(&b.phone.enrollment.candidate,sizeof(b.phone.enrollment.candidate));
    assert(bridge_drained(&b,2404,2));
    assert(bridge_open(&b,2500,PHONE_PROFILE_VALID,true,true,&store)==3);
    assert(bridge_claim(&b,2501,3,3)); f.live_delay=0; f.now=2502;t=REQ(3,3,stage);assert(t);assert(bridge_sent(&b,2503,3,3,t,true));
    assert(bridge_trial(&b,2504,3,4,true));f.now=2602;f.write_delay=2000;save_msg(save,4);
    assert(!REQ(3,3,save));assert(b.stopping&&f.writes==2&&f.reads==2);zero(out,sizeof(out));
    assert(bridge_drained(&b,4603,3));assert(bridge_open(&b,4700,PHONE_PROFILE_VALID,true,true,&store)==4);
    bridge_tick(&b,304700);assert(b.stopping&&!b.phone.window);assert(bridge_drained(&b,304701,4));
    assert(bridge_open(&b,304702,PHONE_PROFILE_VALID,true,true,&store)==5);assert(bridge_claim(&b,304703,5,4));
    f.now=304704;t=REQ(5,4,cancel);assert(t&&out[1]==ROOM_REPLY_CANCELLED&&b.life.phase==SETUP_FINAL);
    bridge_tick(&b,306704);assert(b.stopping);assert(bridge_drained(&b,306705,5));
    assert(bridge_open(&b,306706,PHONE_PROFILE_VALID,true,true,&store)==6);assert(bridge_claim(&b,306707,6,5));
    bridge_lost(&b,306708,5,4);assert(!b.stopping);bridge_lost(&b,306709,6,5);assert(b.stopping);
    assert(bridge_drained(&b,306710,6));assert(bridge_open(&b,306711,PHONE_PROFILE_VALID,true,true,&store)==7);
    bridge_tick(&b,306710);assert(b.stopping&&b.life.failed);assert(bridge_drained(&b,306712,7));
    assert(!bridge_open(&b,306713,PHONE_PROFILE_VALID,true,true,&store));
    puts("room setup bridge PASS");return 0;
}
