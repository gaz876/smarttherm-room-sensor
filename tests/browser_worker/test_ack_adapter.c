#include "completion_adapter.h"
#include <assert.h>
#include <stdio.h>

static unsigned checks;
#define CHECK(x) do { assert(x); ++checks; } while(0)
static uint64_t now;
static bool clock_ok=true;
static bool read_clock(void *ctx,uint64_t *v) { (void)ctx; *v=now; return clock_ok; }
static ack_adapter a;
static mb_reply out;
static uint8_t bytes[8]={42}, copy[8];
#define EVENT(op,w,t,...) ack_step(&a,(mb_event){.action=op,.window=w,.ticket=t,__VA_ARGS__},false,&out,read_clock,NULL)
#define POLL(w,t) ack_step(&a,(mb_event){.window=w,.ticket=t},true,&out,read_clock,NULL)
static uint64_t prepare(uint64_t w) {
    CHECK(EVENT(MB_SUBMIT,w,0,.input=bytes,.size=sizeof bytes)==ACK_OK);
    uint64_t t=out.ticket;
    CHECK(EVENT(MB_TAKE_REQUEST,w,t,.output=copy,.size=sizeof copy)==ACK_OK);
    CHECK(EVENT(MB_PUBLISH,w,t,.input=copy,.size=sizeof copy)==ACK_OK);
    CHECK(POLL(w,t)==ACK_WAIT);
    CHECK(EVENT(MB_SENT,w,t,.flag=true)==ACK_REJECT);
    CHECK(EVENT(MB_TAKE_REPLY,w,t,.output=copy,.size=sizeof copy)==ACK_OK);
    CHECK(POLL(w,t)==ACK_WAIT);
    return t;
}
static void open_window(uint64_t w) {
    CHECK(EVENT(MB_OPEN,w,0,.flag=false)==ACK_OK);
    CHECK(EVENT(MB_STARTED,w,0,.flag=true)==ACK_OK);
}
static void reset(void) { ack_init(&a); now=0; clock_ok=true; open_window(1); }
int main(void) {
    reset(); uint64_t t=prepare(1);
    CHECK(POLL(2,t)==ACK_REJECT);
    CHECK(POLL(1,t+1)==ACK_REJECT);
    CHECK(EVENT(MB_SENT,1,t,.flag=true)==ACK_OK);
    CHECK(EVENT(MB_SUBMIT,1,0,.input=bytes,.size=sizeof bytes)==ACK_WAIT);
    CHECK(POLL(1,t+1)==ACK_REJECT);
    atomic_store(&a.held,true); CHECK(POLL(1,t)==ACK_BUSY); CHECK(out.ticket==0); atomic_store(&a.held,false);
    CHECK(POLL(1,t)==ACK_OK && out.ticket==t);
    CHECK(POLL(1,t)==ACK_REJECT);
    uint64_t u=prepare(1); CHECK(u>t && POLL(1,t)==ACK_REJECT);
    CHECK(EVENT(MB_SENT,1,u,.flag=false)==ACK_REJECT);
    CHECK(POLL(1,u)==ACK_REJECT);
    CHECK(EVENT(MB_DRAINED,1,0,.flag=true,.drained=false)==ACK_REJECT);
    CHECK(EVENT(MB_DRAINED,1,0,.flag=true,.drained=true)==ACK_OK);
    open_window(2); u=prepare(2); CHECK(POLL(1,u)==ACK_REJECT);
    CHECK(EVENT(MB_SENT,2,u,.flag=true)==ACK_OK);
    CHECK(EVENT(MB_STOP,2,0,.flag=false)==ACK_OK); CHECK(POLL(2,u)==ACK_REJECT);
    reset(); t=prepare(1); now=1999;
    CHECK(EVENT(MB_SENT,1,t,.flag=true)==ACK_OK); now=2000;
    CHECK(POLL(1,t)==ACK_REJECT && a.box.life.phase==W_STOP);
    reset(); t=prepare(1); now=2000;
    CHECK(EVENT(MB_SENT,1,t,.flag=true)==ACK_REJECT); CHECK(POLL(1,t)==ACK_REJECT);
    reset(); t=prepare(1); now=5;
    CHECK(EVENT(MB_SENT,1,t,.flag=true)==ACK_OK); now=4;
    CHECK(POLL(1,t)==ACK_REJECT && a.box.life.fault);
    reset(); t=prepare(1); CHECK(EVENT(MB_SENT,1,t,.flag=true)==ACK_OK); clock_ok=false;
    CHECK(POLL(1,t)==ACK_REJECT && a.box.life.fault);
    clock_ok=true; CHECK(POLL(1,t)==ACK_REJECT);
    printf("offline ack adapter: %u checks PASS\n",checks);
    return 0;
}
