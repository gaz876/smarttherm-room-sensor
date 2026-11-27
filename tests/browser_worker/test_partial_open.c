#include "room_phone_channel.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Offline mock: partial start can own a backend even if open returns false.
 * Separate observations represent stop return, callback drain and buffer drain.
 * No SDK, actual transport, credentials or device state is used. */
typedef struct {
    bool resources, stop_returned, callbacks_drained, buffers_drained;
    unsigned opens, closes, receives, sends;
} fake_backend;
static int owner_id, other_id;
static void *current=&owner_id;
static unsigned checks, completions;
#define CHECK(x) do { ++checks; assert(x); } while (0)
void *xTaskGetCurrentTaskHandle(void) { return current; }
bool room_apsta_ready(room_apsta_owner *r,uint64_t w) { return r->ap && w; }
void enroll_wipe(void *p,size_t n) { volatile uint8_t *b=p; while(n--) *b++=0; }
bool room_apsta_task_poll(room_apsta_task *t) { return !t->failed; }
bool room_apsta_task_packet(room_apsta_task *t,bool hs,const uint8_t *p,size_t n,const uint8_t **r,size_t *len) {
    (void)t; (void)hs; (void)p; (void)n; (void)r; (void)len; assert(0); return false;
}
bool room_apsta_task_sent(room_apsta_task *t,bool ok) { (void)t; (void)ok; ++completions; return false; }
static bool open_io(void *v,const esp_netif_t *ap,uint64_t w) {
    fake_backend *b=v; CHECK(ap && w && !b->resources); ++b->opens; b->resources=true; return false;
}
static room_phone_io_result receive_io(void *v,bool *hs,uint8_t *p,size_t cap,size_t *n) {
    fake_backend *b=v; (void)hs; (void)p; (void)cap; (void)n; ++b->receives; return ROOM_PHONE_IO_ERROR;
}
static room_phone_io_result send_io(void *v,const uint8_t *p,size_t n,size_t *written) {
    fake_backend *b=v; (void)p; (void)n; (void)written; ++b->sends; return ROOM_PHONE_IO_ERROR;
}
static bool close_io(void *v) {
    fake_backend *b=v; ++b->closes; CHECK(b->resources);
    if (!b->stop_returned || !b->callbacks_drained || !b->buffers_drained) return false;
    b->resources=false; return true;
}
int main(void) {
    for (unsigned order=0; order<6; ++order) {
        static const unsigned permutations[6][3]={{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}};
        fake_backend b={0}; room_phone_channel c={0}; room_apsta_task t={0}; esp_netif_t ap={0};
        t.ready=true; t.owner=&owner_id; t.session.radio.ap=&ap; t.session.bridge.window=order+1;
        room_phone_io io={&b,open_io,receive_io,send_io,close_io};
        CHECK(!room_phone_channel_open(&c,&t,io));
        CHECK(c.attempted && c.stopping && b.resources && b.opens==1);
        CHECK(!room_phone_channel_poll(&c) && !b.receives && !b.sends);
        CHECK(!room_phone_channel_open(&c,&t,io) && b.opens==1);
        current=&other_id; CHECK(!room_phone_channel_close(&c) && b.closes==0); current=&owner_id;
        CHECK(!room_phone_channel_close(&c) && b.closes==1 && b.resources);
        bool *flags[]={&b.stop_returned,&b.callbacks_drained,&b.buffers_drained};
        for(unsigned i=0;i<3;++i) {
            *flags[permutations[order][i]]=true;
            bool closed=room_phone_channel_close(&c);
            CHECK(closed==(i==2)); CHECK(b.resources==(i!=2));
            CHECK(b.closes==i+2);
        }
        CHECK(!c.attempted && !c.task && !c.reply && !completions);
        CHECK(room_phone_channel_close(&c) && b.closes==4);
        CHECK(!b.receives && !b.sends);
    }
    printf("offline partial start and independent drain permutations: %u checks PASS\n",checks);
    return 0;
}
