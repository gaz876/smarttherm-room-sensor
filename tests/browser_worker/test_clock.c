#define _POSIX_C_SOURCE 200809L
#include "mailbox.h"
#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <time.h>
static mailbox m;
static unsigned checks;
#define CHECK(x) do { assert(x); ++checks; } while(0)
static bool real_clock(void *ctx,uint64_t *out) {
    (void)ctx; struct timespec t;
    if(clock_gettime(CLOCK_MONOTONIC,&t)!=0 || t.tv_sec<0) return false;
    *out=(uint64_t)t.tv_sec*1000+(uint64_t)t.tv_nsec/1000000; return true;
}
typedef struct { uint64_t now; unsigned calls; bool ok; } fake_clock;
static bool fake(void *ctx,uint64_t *out) {
    fake_clock *f=ctx; ++f->calls; *out=f->now; return f->ok;
}
static mb_result apply(mb_event e,mb_reply *r) { return mb_apply_clocked(&m,e,r,real_clock,NULL); }
static void *consumer(void *ctx) {
    (void)ctx; mb_reply r; uint8_t in[8];
    for(uint64_t t=1;t<=10000;t++) {
        while(apply((mb_event){.action=MB_TAKE_REQUEST,.now=UINT64_MAX,.window=1,.ticket=t,.output=in,.size=8},&r)!=MB_OK) sched_yield();
        assert(r.length==8 && r.ticket==t);
        for(size_t i=0;i<8;i++) { assert(in[i]==(uint8_t)(t+i)); in[i]^=0x5a; }
        mb_result rc;
        do { rc=apply((mb_event){.action=MB_PUBLISH,.window=1,.ticket=t,.input=in,.size=8},&r); if(rc==MB_BUSY) sched_yield(); } while(rc==MB_BUSY);
        assert(rc==MB_OK);
    }
    return NULL;
}
int main(void) {
    mb_init(&m); fake_clock f={.now=100,.ok=true}; mb_reply r;
#define F(a,w,t,...) mb_apply_clocked(&m,(mb_event){.action=a,.now=UINT64_MAX,.window=w,.ticket=t,__VA_ARGS__},&r,fake,&f)
    CHECK(F(MB_OPEN,1,0,.flag=false)==MB_OK && m.life.last_time==100 && f.calls==1);
    atomic_store(&m.held,true); CHECK(F(MB_STARTED,1,0,.flag=true)==MB_BUSY && f.calls==1); atomic_store(&m.held,false);
    f.now=101; CHECK(F(MB_STARTED,1,0,.flag=true)==MB_OK);
    uint8_t x=7; CHECK(F(MB_SUBMIT,1,0,.input=&x,.size=1)==MB_OK && r.ticket==1);
    f.now=2100; CHECK(F(MB_TICK,1,0,.flag=false)==MB_OK && m.life.phase==W_REQUEST);
    f.now=2101; CHECK(F(MB_TICK,1,0,.flag=false)==MB_OK && m.life.phase==W_STOP && m.data[0]==0);
    f.now=2102; CHECK(F(MB_DRAINED,1,0,.flag=true,.drained=true)==MB_OK);
    CHECK(F(MB_OPEN,2,0,.flag=false)==MB_OK); f.ok=false;
    CHECK(F(MB_STARTED,2,0,.flag=true)==MB_REJECT && m.life.fault && m.life.phase==W_STOP);
    f.ok=true; CHECK(F(MB_DRAINED,2,0,.flag=true,.drained=true)==MB_REJECT);
    CHECK(F(MB_OPEN,3,0,.flag=false)==MB_REJECT);
    mb_init(&m); f.now=10; CHECK(F(MB_OPEN,1,0,.flag=false)==MB_OK);
    f.now=9; CHECK(F(MB_STARTED,1,0,.flag=true)==MB_REJECT && m.life.fault);
    CHECK(mb_apply_clocked(&m,(mb_event){0},&r,NULL,NULL)==MB_REJECT);
    mb_init(&m); CHECK(atomic_is_lock_free(&m.held));
    CHECK(apply((mb_event){.action=MB_OPEN,.window=1},&r)==MB_OK);
    CHECK(apply((mb_event){.action=MB_STARTED,.window=1,.flag=true},&r)==MB_OK);
    pthread_t th; CHECK(pthread_create(&th,NULL,consumer,NULL)==0);
    for(uint64_t t=1;t<=10000;t++) {
        uint8_t b[8],reply[8]; for(size_t i=0;i<8;i++) b[i]=(uint8_t)(t+i);
        mb_result rc; do { rc=apply((mb_event){.action=MB_SUBMIT,.window=1,.input=b,.size=8},&r); if(rc==MB_BUSY) sched_yield(); } while(rc==MB_BUSY);
        assert(rc==MB_OK && r.ticket==t);
        while(apply((mb_event){.action=MB_TAKE_REPLY,.window=1,.ticket=t,.output=reply,.size=8},&r)!=MB_OK) sched_yield();
        assert(r.length==8); for(size_t i=0;i<8;i++) assert(reply[i]==(uint8_t)(b[i]^0x5a));
        do { rc=apply((mb_event){.action=MB_SENT,.window=1,.ticket=t,.flag=true},&r); if(rc==MB_BUSY) sched_yield(); } while(rc==MB_BUSY);
        assert(rc==MB_OK);
    }
    CHECK(pthread_join(th,NULL)==0); CHECK(m.life.phase==W_IDLE && !m.life.fault);
    printf("clocked mailbox: %u checks + 10000 real-clock round trips PASS\n",checks);
}
