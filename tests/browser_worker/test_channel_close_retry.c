#include "room_phone_channel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Offline fault fixture: no socket, SDK, or credentials. A successful backend
 * close consumes the fixture handle, so another invocation is invalid. */
static unsigned checks, close_calls, sent_calls;
static int owner_id, other_id;
static void *current_owner=&owner_id;
static bool opened, allow_close, allow_finish;
static uint8_t response=7;
#define CHECK(x) do { ++checks; if(!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while(0)
void *xTaskGetCurrentTaskHandle(void) { return current_owner; }
bool room_apsta_ready(room_apsta_owner *r,uint64_t w) { return r->ap && w; }
void enroll_wipe(void *p,size_t n) { volatile uint8_t *b=p; while(n--) *b++=0; }
bool room_apsta_task_poll(room_apsta_task *t) { return !t->failed; }
bool room_apsta_task_packet(room_apsta_task *t,bool h,const uint8_t *p,size_t n,const uint8_t **r,size_t *len) {
    CHECK(h && n==1 && p[0]==3); t->session.transport.pending=true; *r=&response; *len=1; return true;
}
bool room_apsta_task_sent(room_apsta_task *t,bool ok) {
    ++sent_calls; CHECK(!ok && t->session.transport.pending && !t->session.transport.dispatching);
    if(allow_finish) t->session.transport.pending=false;
    return false; /* Aborted sends must not be reported as successful. */
}
static bool open_io(void *p,const esp_netif_t *ap,uint64_t w) {
    (void)p; opened=ap && w; return opened;
}
static room_phone_io_result receive_io(void *p,bool *hs,uint8_t *b,size_t cap,size_t *n) {
    (void)p; CHECK(opened && cap>0); *hs=true; b[0]=3; *n=1; return ROOM_PHONE_IO_OK;
}
static room_phone_io_result send_io(void *p,const uint8_t *b,size_t n,size_t *written) {
    (void)p; (void)b; (void)n; CHECK(opened); *written=0; return ROOM_PHONE_IO_WAIT;
}
static bool close_io(void *p) {
    (void)p; ++close_calls; if(!opened || !allow_close) return false;
    opened=false; return true;
}
static void begin(room_phone_channel *c,room_apsta_task *t,esp_netif_t *ap) {
    *c=(room_phone_channel){0}; *t=(room_apsta_task){0};
    current_owner=&owner_id; close_calls=sent_calls=0; opened=false; allow_close=allow_finish=false;
    t->ready=true; t->owner=&owner_id; t->session.radio.ap=ap; t->session.bridge.window=1;
    CHECK(room_phone_channel_open(c,t,(room_phone_io){NULL,open_io,receive_io,send_io,close_io}));
    CHECK(room_phone_channel_poll(c) && c->reply==&response && t->session.transport.pending);
}
int main(void) {
    room_phone_channel c; room_apsta_task t; esp_netif_t ap={0};
    begin(&c,&t,&ap);
    current_owner=&other_id; CHECK(!room_phone_channel_close(&c) && !close_calls);
    current_owner=&owner_id;
    CHECK(!room_phone_channel_close(&c) && close_calls==1 && !sent_calls);
    CHECK(!room_phone_channel_poll(&c));
    allow_close=true;
    CHECK(!room_phone_channel_close(&c) && close_calls==2 && sent_calls==1 && !opened);
    CHECK(c.reply && t.session.transport.pending);
    allow_finish=true;
    CHECK(room_phone_channel_close(&c) && close_calls==2 && sent_calls==2);
    CHECK(!c.task && !c.reply && !t.session.transport.pending);
    CHECK(room_phone_channel_close(&c) && close_calls==2);
    begin(&c,&t,&ap); allow_close=allow_finish=true;
    t.session.transport.dispatching=true;
    CHECK(!room_phone_channel_close(&c) && close_calls==1 && !sent_calls);
    CHECK(c.reply && t.session.transport.pending && !opened);
    t.session.transport.dispatching=false;
    CHECK(room_phone_channel_close(&c) && close_calls==1 && sent_calls==1);
    begin(&c,&t,&ap); allow_close=allow_finish=true;
    t.session.bridge.window=2;
    CHECK(!room_phone_channel_close(&c) && close_calls==1 && !sent_calls);
    CHECK(!room_phone_channel_close(&c) && close_calls==1 && !sent_calls);
    CHECK(c.reply && t.session.transport.pending && c.stopping);
    printf("offline close retry: %u checks PASS\n",checks); return 0;
}
