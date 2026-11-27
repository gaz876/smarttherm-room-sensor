#include "room_phone_channel.h"

static bool owner(const room_phone_channel *c) {
    return c&&c->task&&c->task->ready&&!c->task->failed&&
        c->task->owner==xTaskGetCurrentTaskHandle();
}
static bool active(const room_phone_channel *c) {
    return owner(c)&&c->attempted&&!c->stopping&&c->window&&
        c->window==c->task->session.bridge.window;
}
static bool fail(room_phone_channel *c) {
    c->stopping=true;
    return false;
}
bool room_phone_channel_open(room_phone_channel *c,room_apsta_task *t,room_phone_io io) {
    if(!c||c->attempted||c->task||!t||!t->ready||t->failed||
        t->owner!=xTaskGetCurrentTaskHandle()||!io.open||!io.receive||!io.send||!io.close||
        !t->session.bridge.window||!t->session.radio.ap||
        !room_apsta_ready(&t->session.radio,t->session.bridge.window))return false;
    c->io=io;c->task=t;c->window=t->session.bridge.window;c->attempted=true;
    if(!io.open(io.ctx,t->session.radio.ap,c->window))return fail(c);
    return true;
}
bool room_phone_channel_poll(room_phone_channel *c) {
    if(!active(c))return false;
    /* Advance SDK events/deadlines before touching backend I/O, including
     * when a response is pending. Do not dispatch while a marker is pending. */
    if(!room_apsta_task_poll(c->task))return fail(c);
    if(c->reply){
        size_t written=0,remaining=c->length-c->offset;
        room_phone_io_result r=c->io.send(c->io.ctx,c->reply+c->offset,remaining,&written);
        if(r==ROOM_PHONE_IO_WAIT&&written==0)return true;
        if(r!=ROOM_PHONE_IO_OK||!written||written>remaining)return fail(c);
        c->offset+=written;
        if(c->offset<c->length)return true;
        c->reply=NULL;c->length=c->offset=0;
        if(!room_apsta_task_sent(c->task,true))return fail(c);
        return true;
    }
    if(c->task->pending)return true;
    bool handshake=false;size_t n=0;
    room_phone_io_result r=c->io.receive(c->io.ctx,&handshake,c->packet,sizeof(c->packet),&n);
    if(r==ROOM_PHONE_IO_WAIT&&n==0){enroll_wipe(c->packet,sizeof(c->packet));return true;}
    bool ok=r==ROOM_PHONE_IO_OK&&n&&n<=sizeof(c->packet)&&
        room_apsta_task_packet(c->task,handshake,c->packet,n,&c->reply,&c->length);
    enroll_wipe(c->packet,sizeof(c->packet));
    if(!ok||!c->reply||!c->length||c->length>ROOM_PHONE_PACKET_MAX)return fail(c);
    return true;
}
bool room_phone_channel_close(room_phone_channel *c) {
    if(!c)return false;
    if(!c->attempted)return !c->task&&!c->reply;
    if(!owner(c))return false;
    c->stopping=true;
    if(!c->io.close(c->io.ctx))return false;
    /* Backend is quiescent now. Only now return the borrowed buffer; a
     * transport loss cannot undo a verified save. No stale generation use. */
    if(c->window!=c->task->session.bridge.window)return false;
    if(c->task->session.transport.pending)(void)room_apsta_task_sent(c->task,false);
    if(c->task->session.transport.pending||c->task->session.transport.dispatching)return false;
    enroll_wipe(c->packet,sizeof(c->packet));
    *c=(room_phone_channel){0};return true;
}
