#include "room_measure.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef struct {
    uint64_t now;
    unsigned resets, starts, reads, waits;
    uint8_t frame[6];
    bool fail_reset, fail_start, fail_read, corrupt, short_wait, backwards;
    uint32_t extra_delay;
    uint8_t phase;
} mock;
static bool wr(void *ctx,uint8_t cmd) {
    mock *m=ctx;
    if(cmd==0x94) {m->resets++;m->phase=1;return !m->fail_reset;}
    assert(cmd==0xfd && (m->phase==0 || m->phase==2 || m->phase==4));
    m->starts++;m->phase=3;return !m->fail_start;
}
static bool rd(void *ctx,uint8_t *data,size_t len) {
    mock *m=ctx;assert(len==6 && m->phase==4);m->reads++;m->phase=0;
    if(m->fail_read) return false;
    memcpy(data,m->frame,6);
    if(m->corrupt) data[5]^=1;
    if(m->backwards) m->now=0;
    return true;
}
static void wait(void *ctx,uint32_t ms) {
    mock *m=ctx;m->waits++;
    assert((m->phase==1 && ms>=1) || (m->phase==3 && ms>=9));
    m->phase++;m->now+=(m->short_wait ? 0 : ms)+m->extra_delay;
}
static uint64_t clock_ms(void *ctx) {return ((mock*)ctx)->now;}
static void frame(mock *m) {m->frame[0]=0x80;m->frame[3]=0x80;m->frame[2]=sht40_crc(0x80,0);m->frame[5]=sht40_crc(0x80,0);}
static void no_data(room_measure *m,uint64_t now,room_status status) {
    sht40_sample out={1,2,3,true};assert(room_snapshot(m,now,&out)==status);
    assert(!out.valid && out.temperature==0 && out.humidity==0 && out.measured_ms==0);
}
int main(void) {
    room_measure m; mock hw={0};frame(&hw);
    room_bus bus={&hw,wr,rd,wait,clock_ms};sht40_sample out;
    room_init(&m);assert(m.interval_ms==10000);no_data(&m,0,ROOM_NOT_READY);
    assert(room_poll(&m,&bus));assert(hw.resets==1 && hw.starts==1 && hw.reads==1 && hw.now==12);
    assert(room_snapshot(&m,12,&out)==ROOM_OK && out.valid && out.measured_ms==0);
    no_data(&m,11,ROOM_STALE);
    hw.now=9999;assert(!room_poll(&m,&bus));assert(room_snapshot(&m,9999,&out)==ROOM_OK);
    no_data(&m,10000,ROOM_STALE);hw.now=10000;assert(room_poll(&m,&bus));assert(hw.resets==1 && hw.starts==2);
    hw.now=20000;hw.fail_start=true;assert(room_poll(&m,&bus));no_data(&m,hw.now,ROOM_IO_ERROR);
    hw.fail_start=false;hw.now=30000;hw.fail_read=true;assert(room_poll(&m,&bus));no_data(&m,hw.now,ROOM_IO_ERROR);
    hw.fail_read=false;hw.now=40000;hw.fail_reset=true;unsigned starts=hw.starts;
    assert(room_poll(&m,&bus));assert(hw.starts==starts);no_data(&m,hw.now,ROOM_IO_ERROR);
    hw.fail_reset=false;hw.now=50000;hw.corrupt=true;assert(room_poll(&m,&bus));no_data(&m,hw.now,ROOM_INVALID);
    hw.corrupt=false;hw.now=60000;assert(room_poll(&m,&bus));assert(room_snapshot(&m,hw.now,&out)==ROOM_OK);
    assert(!room_configure(&m,0,0));assert(!room_configure(&m,999,0));assert(!room_configure(&m,3600001,0));
    assert(!room_configure(&m,1000,NAN));assert(!room_configure(&m,1000,11));
    assert(room_snapshot(&m,hw.now,&out)==ROOM_OK);
    assert(room_configure(&m,1000,1));no_data(&m,hw.now,ROOM_NOT_READY);
    assert(room_poll(&m,&bus));assert(room_snapshot(&m,hw.now,&out)==ROOM_OK && fabs(out.temperature-43.501335)<0.001);
    assert(fabs(out.humidity-56.500954)<0.001);
    uint64_t prior=m.last_start_ms;hw.now=prior+999;assert(!room_poll(&m,&bus));
    hw.now=prior+1000;assert(room_poll(&m,&bus));hw.now=prior-1;
    assert(!room_poll(&m,&bus));no_data(&m,hw.now,ROOM_INVALID);
    assert(room_poll(&m,&bus));assert(room_snapshot(&m,hw.now,&out)==ROOM_OK);
    room_init(&m);hw.now=0;no_data(&m,0,ROOM_NOT_READY);unsigned resets=hw.resets;
    assert(room_poll(&m,&bus));assert(hw.resets==resets+1);
    assert(room_configure(&m,1000,0));hw.extra_delay=1200;
    assert(room_poll(&m,&bus));no_data(&m,hw.now,ROOM_INVALID);hw.extra_delay=0;
    hw.now+=1000;hw.backwards=true;assert(room_poll(&m,&bus));no_data(&m,hw.now,ROOM_INVALID);
    hw.backwards=false;assert(!room_poll(&m,&bus));assert(room_poll(&m,&bus));
    assert(room_snapshot(&m,hw.now,&out)==ROOM_OK);
    assert(room_configure(&m,1000,0));hw.short_wait=true;
    assert(room_poll(&m,&bus));no_data(&m,hw.now,ROOM_INVALID);hw.short_wait=false;
    hw.now+=1000;assert(room_poll(&m,&bus));assert(room_snapshot(&m,hw.now,&out)==ROOM_OK);
    hw.now=m.last_start_ms+1;assert(!room_poll(&m,&bus));no_data(&m,hw.now,ROOM_INVALID);
    assert(room_poll(&m,&bus));assert(room_snapshot(&m,hw.now,&out)==ROOM_OK);
    assert(!room_poll(&m,NULL));no_data(&m,hw.now,ROOM_IO_ERROR);
    assert(room_snapshot(NULL,0,&out)==ROOM_NOT_READY);assert(room_snapshot(&m,0,NULL)==ROOM_INVALID);
    assert(room_configure(&m,3600000,-10));assert(room_poll(&m,&bus));assert(room_snapshot(&m,hw.now,&out)==ROOM_OK);
    hw.now+=7200000;starts=hw.starts;assert(room_poll(&m,&bus));assert(hw.starts==starts+1);assert(!room_poll(&m,&bus));
    puts("PASS: restart, reset/command/read failures, CRC, recovery, cadence, stale, correction, clock rollback, late/early I/O, no catch-up");
    return 0;
}
