#define _POSIX_C_SOURCE 200809L
#include "stop_gate_adapter.h"
#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>

static stop_gate_adapter g;
static atomic_bool entered,release_clock;
static uint64_t now;
static unsigned checks;
#define CHECK(x) do{++checks;assert(x);}while(0)
static bool normal_clock(void *ctx,uint64_t *v){(void)ctx;*v=now;return true;}
static bool held_clock(void *ctx,uint64_t *v){
    atomic_store(&entered,true);
    while(!atomic_load(&release_clock))sched_yield();
    return normal_clock(ctx,v);
}
static mb_event event;
static mb_reply result;
static ack_result rc;
static bool receipt;
static void *traffic(void *p){(void)p;rc=gate_step(&g,event,receipt,&result,held_clock,NULL);return NULL;}
static ack_result step(mb_event e,bool r,mb_reply *out){return gate_step(&g,e,r,out,normal_clock,NULL);}
static ack_result close_at(uint64_t w,uint64_t deadline,bool exited,bool drained){return gate_close_until(&g,w,deadline,exited,drained,normal_clock,NULL);}
static void race(uint64_t w,mb_event e,bool r,bool mismatch){
    pthread_t t;event=e;receipt=r;atomic_store(&entered,false);atomic_store(&release_clock,false);
    CHECK(pthread_create(&t,NULL,traffic,NULL)==0);
    while(!atomic_load(&entered))sched_yield();
    CHECK(close_at(w-1,5000,true,true)==ACK_REJECT);
    CHECK(atomic_load(&g.gate)==GATE_BUSY);
    CHECK(close_at(w,5000,false,false)==ACK_BUSY);
    CHECK(atomic_load(&g.gate)==(GATE_STOP|GATE_BUSY));
    mb_reply out;uint8_t b=1;
    CHECK(step((mb_event){.action=MB_SUBMIT,.window=w,.input=&b,.size=1},false,&out)==ACK_REJECT);
    CHECK(step((mb_event){.window=w,.ticket=e.ticket},true,&out)==ACK_REJECT);
    if(mismatch)CHECK(close_at(w,4999,true,true)==ACK_REJECT);
    atomic_store(&release_clock,true);CHECK(pthread_join(t,NULL)==0);
    CHECK(rc==ACK_REJECT&&!result.ticket&&!result.length);
    CHECK(atomic_load(&g.gate)==GATE_STOP);
    CHECK(step((mb_event){.action=MB_SUBMIT,.window=w,.input=&b,.size=1},false,&out)==ACK_REJECT);
    if(mismatch){CHECK(close_at(w,5000,true,true)==ACK_REJECT);CHECK(!gate_open(&g,w+1,normal_clock,NULL));return;}
    CHECK(close_at(w,5000,true,false)==ACK_WAIT);
    CHECK(close_at(w,5000,true,true)==ACK_OK);
}
int main(void){
    atomic_init(&entered,false);atomic_init(&release_clock,false);gate_init(&g);
    uint8_t input[4096],copy[4096];memset(input,0x5a,sizeof input);
    mb_reply out;uint64_t last=0;
    for(uint64_t w=2;w<602;w++){
        CHECK(gate_open(&g,w,normal_clock,NULL));
        if(w%6==0){race(w,(mb_event){.action=MB_SUBMIT,.window=w,.input=input,.size=sizeof input},false,false);continue;}
        CHECK(step((mb_event){.action=MB_SUBMIT,.window=w,.input=input,.size=sizeof input},false,&out)==ACK_OK);
        uint64_t ticket=out.ticket;CHECK(ticket>last);last=ticket;
        memset(copy,0xa5,sizeof copy);
        mb_event take={.action=MB_TAKE_REQUEST,.window=w,.ticket=ticket,.output=copy,.size=sizeof copy};
        if(w%6==1){race(w,take,false,false);for(size_t i=0;i<sizeof copy;i++)CHECK(copy[i]==0);continue;}
        CHECK(step(take,false,&out)==ACK_OK&&memcmp(input,copy,sizeof copy)==0);
        mb_event publish={.action=MB_PUBLISH,.window=w,.ticket=ticket,.input=input,.size=sizeof input};
        if(w%6==2){race(w,publish,false,false);continue;}
        CHECK(step(publish,false,&out)==ACK_OK);
        mb_event reply={.action=MB_TAKE_REPLY,.window=w,.ticket=ticket,.output=copy,.size=sizeof copy};
        if(w%6==3){race(w,reply,false,false);for(size_t i=0;i<sizeof copy;i++)CHECK(copy[i]==0);continue;}
        CHECK(step(reply,false,&out)==ACK_OK);
        mb_event sent={.action=MB_SENT,.window=w,.ticket=ticket,.flag=true};
        if(w%6==4){race(w,sent,false,false);continue;}
        CHECK(step(sent,false,&out)==ACK_OK);
        race(w,(mb_event){.window=w,.ticket=ticket},true,false);
    }
    CHECK(gate_open(&g,602,normal_clock,NULL));
    race(602,(mb_event){.action=MB_SUBMIT,.window=602,.input=input,.size=1},false,true);
    printf("STOP_GATE_PASS races=601 checks=%u\n",checks);return 0;
}
