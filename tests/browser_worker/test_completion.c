#include "mailbox.h"
#include <assert.h>
#include <stdio.h>

/* Offline integration fixture. No network, SDK, or actual sender.
 * Separate local completion receipt is a proposed contract, not a runtime API. */
static unsigned checks;
#define CHECK(x) do { assert(x); ++checks; } while (0)
typedef struct {
    mailbox box;
    uint64_t now, window, ticket;
    mb_reply reply;
    uint8_t input[8], output[8];
    bool copied, receipt;
} fixture;
static bool clock_read(void *ctx, uint64_t *ms) {
    *ms=((fixture *)ctx)->now; return true;
}
static mb_result call(fixture *f, mb_event e) {
    return mb_apply_clocked(&f->box,e,&f->reply,clock_read,f);
}
#define EVENT(f,a,t,...) call(f,(mb_event){.action=a,.window=(f)->window,.ticket=t,__VA_ARGS__})
static void start(fixture *f) {
    memset(f,0,sizeof *f); mb_init(&f->box); f->window=1;
    CHECK(EVENT(f,MB_OPEN,0,.flag=false)==MB_OK);
    CHECK(EVENT(f,MB_STARTED,0,.flag=true)==MB_OK);
    f->input[0]=42;
    CHECK(EVENT(f,MB_SUBMIT,0,.input=f->input,.size=sizeof f->input)==MB_OK);
    f->ticket=f->reply.ticket;
    CHECK(EVENT(f,MB_TAKE_REQUEST,f->ticket,.output=f->output,.size=sizeof f->output)==MB_OK);
    CHECK(EVENT(f,MB_PUBLISH,f->ticket,.input=f->output,.size=sizeof f->output)==MB_OK);
}
static void take(fixture *f) {
    CHECK(EVENT(f,MB_TAKE_REPLY,f->ticket,.output=f->output,.size=sizeof f->output)==MB_OK);
    f->copied=true;
}
/* Only the simulated sender's completion event may create a receipt. */
static bool completed(fixture *f, uint64_t ticket, bool ok) {
    if(!f->copied || f->receipt || ticket!=f->ticket) return false;
    mb_result rc=EVENT(f,MB_SENT,ticket,.flag=ok);
    if(rc!=MB_OK) return false;
    f->receipt=true; return true;
}
int main(void) {
    fixture f;
    start(&f);
    CHECK(!completed(&f,f.ticket,true));
    CHECK(EVENT(&f,MB_SENT,f.ticket,.flag=true)==MB_REJECT);
    take(&f);
    CHECK(f.output[0]==42 && !f.receipt && f.box.life.phase==W_REPLY);
    CHECK(EVENT(&f,MB_TAKE_REPLY,f.ticket,.output=f.output,.size=sizeof f.output)==MB_REJECT);
    CHECK(EVENT(&f,MB_SUBMIT,0,.input=f.input,.size=sizeof f.input)==MB_REJECT);
    CHECK(!completed(&f,f.ticket+1,true));
    CHECK(!f.receipt);
    CHECK(completed(&f,f.ticket,true) && f.receipt && f.box.life.phase==W_IDLE);
    CHECK(!completed(&f,f.ticket,true));
    CHECK(EVENT(&f,MB_TAKE_REPLY,f.ticket,.output=f.output,.size=sizeof f.output)==MB_REJECT);
    start(&f); take(&f);
    CHECK(!completed(&f,f.ticket,false) && !f.receipt && f.box.life.phase==W_STOP);
    CHECK(EVENT(&f,MB_DRAINED,0,.flag=true,.drained=false)==MB_REJECT);
    CHECK(EVENT(&f,MB_DRAINED,0,.flag=true,.drained=true)==MB_OK);
    start(&f); take(&f); f.now=2000;
    CHECK(!completed(&f,f.ticket,true) && !f.receipt && f.box.life.phase==W_STOP);
    CHECK(EVENT(&f,MB_DRAINED,0,.flag=false,.drained=true)==MB_REJECT);
    f.now=4000;
    CHECK(EVENT(&f,MB_DRAINED,0,.flag=true,.drained=true)==MB_REJECT && f.box.life.phase==W_LOCKED);
    start(&f); take(&f);
    CHECK(EVENT(&f,MB_STOP,0,.flag=false)==MB_OK);
    CHECK(!completed(&f,f.ticket,true) && !f.receipt);
    CHECK(EVENT(&f,MB_DRAINED,0,.flag=true,.drained=false)==MB_REJECT);
    CHECK(EVENT(&f,MB_DRAINED,0,.flag=true,.drained=true)==MB_OK);
    CHECK(f.box.life.phase==W_DOWN);
    printf("offline completion contract: %u checks PASS\n",checks);
    return 0;
}
