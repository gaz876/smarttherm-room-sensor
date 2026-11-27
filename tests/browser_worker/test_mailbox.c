#include "mailbox.h"
#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
static unsigned checks;
#define CHECK(x) do { assert(x); ++checks; } while(0)
static mb_reply r;
static mailbox m;
static uint8_t src[MB_CAP], dst[MB_CAP];
#define DO(a,n,w,t,...) mb_apply(&m,(mb_event){.action=a,.now=n,.window=w,.ticket=t,__VA_ARGS__},&r)
static bool zero(const uint8_t *p,size_t n) { for(size_t i=0;i<n;i++) if(p[i]) return false; return true; }
static void *thread_worker(void *arg) {
    (void)arg; uint8_t in[8],out[8]; mb_reply reply;
    for(uint64_t t=1;t<=10000;t++) {
        mb_result rc;
        do { rc=mb_apply(&m,(mb_event){.action=MB_TAKE_REQUEST,.now=1,.window=1,.ticket=t,.output=in,.size=sizeof in},&reply); if(rc!=MB_OK) sched_yield(); } while(rc!=MB_OK);
        assert(reply.length==8 && reply.ticket==t); for(size_t i=0;i<8;i++) { assert(in[i]==(uint8_t)(t+i)); out[i]=(uint8_t)(in[i]^0x5a); }
        do { rc=mb_apply(&m,(mb_event){.action=MB_PUBLISH,.now=1,.window=1,.ticket=t,.input=out,.size=sizeof out},&reply); if(rc!=MB_OK) sched_yield(); } while(rc==MB_BUSY);
        assert(rc==MB_OK);
    }
    return NULL;
}
int main(void) {
    mb_init(&m);
    CHECK(DO(MB_OPEN,0,1,0,.flag=false)==MB_OK);
    CHECK(DO(MB_STARTED,1,1,0,.flag=true)==MB_OK);
    memset(src,0x35,sizeof src); memset(dst,0x99,sizeof dst);
    CHECK(DO(MB_SUBMIT,2,1,0,.input=src,.size=4097)==MB_REJECT);
    CHECK(DO(MB_SUBMIT,2,1,0,.input=src,.size=0)==MB_REJECT);
    CHECK(DO(MB_SUBMIT,2,1,0,.input=src,.size=4096,.flag=true)==MB_OK && r.ticket==1);
    memset(src,0,sizeof src);
    CHECK(DO(MB_SUBMIT,3,1,0,.input=src,.size=1)==MB_REJECT);
    CHECK(DO(MB_TAKE_REQUEST,3,2,1,.output=dst,.size=4096)==MB_REJECT);
    CHECK(DO(MB_TAKE_REQUEST,3,1,2,.output=dst,.size=4096)==MB_REJECT);
    CHECK(DO(MB_TAKE_REQUEST,3,1,1,.output=dst,.size=4095)==MB_REJECT && dst[0]==0x99);
    CHECK(DO(MB_PUBLISH,3,1,1,.input=src,.size=1)==MB_REJECT);
    CHECK(DO(MB_TAKE_REQUEST,4,1,1,.output=dst,.size=4096)==MB_OK && r.length==4096 && r.handshake);
    CHECK(dst[0]==0x35 && dst[4095]==0x35 && zero(m.data,MB_CAP));
    CHECK(DO(MB_TAKE_REQUEST,4,1,1,.output=dst,.size=4096)==MB_REJECT);
    CHECK(DO(MB_PUBLISH,5,1,1,.input=dst,.size=4096)==MB_OK);
    memset(dst,0,sizeof dst);
    CHECK(DO(MB_SENT,5,1,1,.flag=true)==MB_REJECT);
    CHECK(DO(MB_TAKE_REPLY,6,1,1,.output=dst,.size=4096)==MB_OK && dst[4095]==0x35);
    CHECK(zero(m.data,MB_CAP)); CHECK(DO(MB_SENT,7,1,1,.flag=true)==MB_OK);
    CHECK(DO(MB_SUBMIT,8,1,0,.input=dst,.size=1)==MB_OK && r.ticket==2);
    CHECK(DO(MB_TICK,2008,1,0,.flag=false)==MB_OK && m.life.phase==W_STOP && zero(m.data,MB_CAP));
    CHECK(DO(MB_TAKE_REQUEST,2009,1,2,.output=dst,.size=4096)==MB_REJECT);
    CHECK(DO(MB_DRAINED,2010,1,0,.flag=true,.drained=false)==MB_REJECT);
    CHECK(DO(MB_DRAINED,2011,1,0,.flag=true,.drained=true)==MB_OK);
    CHECK(DO(MB_OPEN,2012,2,0,.flag=false)==MB_OK);
    CHECK(DO(MB_STARTED,2013,2,0,.flag=true)==MB_OK);
    CHECK(DO(MB_SUBMIT,2014,2,0,.input=dst,.size=1)==MB_OK && r.ticket==3);
    CHECK(DO(MB_STOP,2015,1,0,.flag=false)==MB_REJECT);
    CHECK(DO(MB_STOP,2015,2,0,.flag=false)==MB_OK && zero(m.data,MB_CAP));
    CHECK(DO(MB_DRAINED,4015,2,0,.flag=true,.drained=true)==MB_REJECT && m.life.phase==W_LOCKED);
    atomic_store(&m.held,true); CHECK(DO(MB_TICK,4016,2,0,.flag=false)==MB_BUSY); atomic_store(&m.held,false);
    /* New isolated fixture, both threads share a fixed monotonic test timestamp. */
    mb_init(&m); CHECK(DO(MB_OPEN,0,1,0,.flag=false)==MB_OK); CHECK(DO(MB_STARTED,1,1,0,.flag=true)==MB_OK);
    pthread_t th; CHECK(pthread_create(&th,NULL,thread_worker,NULL)==0);
    for(uint64_t t=1;t<=10000;t++) {
        mb_result rc; for(size_t i=0;i<8;i++) src[i]=(uint8_t)(t+i);
        do { rc=DO(MB_SUBMIT,1,1,0,.input=src,.size=8); if(rc==MB_BUSY) sched_yield(); } while(rc==MB_BUSY);
        assert(rc==MB_OK && r.ticket==t);
        do { rc=DO(MB_TAKE_REPLY,1,1,t,.output=dst,.size=8); if(rc!=MB_OK) sched_yield(); } while(rc!=MB_OK);
        assert(r.length==8); for(size_t i=0;i<8;i++) assert(dst[i]==(uint8_t)(src[i]^0x5a));
        do { rc=DO(MB_SENT,1,1,t,.flag=true); if(rc==MB_BUSY) sched_yield(); } while(rc==MB_BUSY);
        assert(rc==MB_OK);
    }
    CHECK(pthread_join(th,NULL)==0); CHECK(m.life.phase==W_IDLE && zero(m.data,MB_CAP));
    printf("mailbox: %u checks + 10000 threaded round trips PASS\n",checks); return 0;
}
