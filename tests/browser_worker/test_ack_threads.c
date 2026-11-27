#define _POSIX_C_SOURCE 200809L
#include "completion_adapter.h"
#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <time.h>

enum { ROUNDS=10000, WINDOW=1 };
static ack_adapter adapter;
static atomic_uint owner_waiting;
static bool monotonic_ms(void *ctx,uint64_t *value) {
    (void)ctx;
    struct timespec ts;
    if(clock_gettime(CLOCK_MONOTONIC,&ts)!=0 || ts.tv_sec<0) return false;
    *value=(uint64_t)ts.tv_sec*1000u+(uint64_t)ts.tv_nsec/1000000u;
    return true;
}
/* Bounded test harness only; no production retry loop is proposed. */
static ack_result attempt(mb_event e,bool receipt,mb_reply *out,bool wait) {
    uint64_t start,now;
    assert(monotonic_ms(NULL,&start));
    for(;;) {
        ack_result r=ack_step(&adapter,e,receipt,out,monotonic_ms,NULL);
        if(r!=ACK_BUSY && (!wait || r==ACK_OK)) return r;
        assert(monotonic_ms(NULL,&now) && now-start<1800);
        sched_yield();
    }
}
static void wait_for_owner(unsigned i) {
    uint64_t start,now;
    assert(monotonic_ms(NULL,&start));
    while(atomic_load_explicit(&owner_waiting,memory_order_acquire)<i) {
        assert(monotonic_ms(NULL,&now) && now-start<1800);
        sched_yield();
    }
}
static void *worker(void *unused) {
    (void)unused;
    mb_reply out;
    for(unsigned i=1;i<=ROUNDS;i++) {
        uint8_t input[1]={(uint8_t)i},reply[1]={0};
        assert(attempt((mb_event){.action=MB_SUBMIT,.window=WINDOW,.input=input,.size=sizeof input},false,&out,true)==ACK_OK);
        assert(out.ticket==i);
        assert(attempt((mb_event){.action=MB_TAKE_REPLY,.window=WINDOW,.ticket=i,.output=reply,.size=sizeof reply},false,&out,true)==ACK_OK);
        assert(out.ticket==i && out.length==1 && reply[0]==(uint8_t)(input[0]^0x5a));
        wait_for_owner(i);
        assert(attempt((mb_event){.action=MB_SENT,.window=WINDOW,.ticket=i,.flag=true},false,&out,false)==ACK_OK);
    }
    return NULL;
}
static void *owner(void *unused) {
    (void)unused;
    mb_reply out;
    for(unsigned i=1;i<=ROUNDS;i++) {
        uint8_t input[1]={0},reply[1];
        assert(attempt((mb_event){.action=MB_TAKE_REQUEST,.window=WINDOW,.ticket=i,.output=input,.size=sizeof input},false,&out,true)==ACK_OK);
        assert(out.ticket==i && out.length==1 && input[0]==(uint8_t)i);
        reply[0]=(uint8_t)(input[0]^0x5a);
        assert(attempt((mb_event){.action=MB_PUBLISH,.window=WINDOW,.ticket=i,.input=reply,.size=sizeof reply},false,&out,false)==ACK_OK);
        mb_event poll={.window=WINDOW,.ticket=i};
        assert(attempt(poll,true,&out,false)==ACK_WAIT && out.ticket==0);
        atomic_store_explicit(&owner_waiting,i,memory_order_release);
        assert(attempt(poll,true,&out,true)==ACK_OK && out.ticket==i);
        assert(attempt(poll,true,&out,false)==ACK_REJECT && out.ticket==0);
    }
    return NULL;
}
int main(void) {
    ack_init(&adapter); atomic_init(&owner_waiting,0);
    mb_reply out;
    assert(attempt((mb_event){.action=MB_OPEN,.window=WINDOW},false,&out,false)==ACK_OK);
    assert(attempt((mb_event){.action=MB_STARTED,.window=WINDOW,.flag=true},false,&out,false)==ACK_OK);
    pthread_t a,b;
    assert(pthread_create(&a,NULL,owner,NULL)==0);
    assert(pthread_create(&b,NULL,worker,NULL)==0);
    assert(pthread_join(a,NULL)==0 && pthread_join(b,NULL)==0);
    assert(attempt((mb_event){.action=MB_STOP,.window=WINDOW},false,&out,false)==ACK_OK);
    assert(attempt((mb_event){.action=MB_DRAINED,.window=WINDOW,.flag=true,.drained=false},false,&out,false)==ACK_REJECT);
    assert(attempt((mb_event){.action=MB_DRAINED,.window=WINDOW,.flag=true,.drained=true},false,&out,false)==ACK_OK);
    printf("offline two-thread ack adapter: %u round trips PASS\n",ROUNDS);
    return 0;
}
