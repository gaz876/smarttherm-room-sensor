#ifndef ROOM_SETUP_DISPATCH_H
#define ROOM_SETUP_DISPATCH_H
#include "room_phone_setup.h"
#include "room_setup_request.h"

/* Host-only candidate. One owner task, monotonic milliseconds. The session
 * comes from an authenticated transport, not the payload. The live callback
 * must synchronously observe the actual trial's link and fresh DHCP lease.
 * No network I/O, cryptography or transport authentication in this module. */
typedef enum {
    ROOM_REPLY_OK=1, ROOM_REPLY_REJECTED=2, ROOM_REPLY_WAIT=3,
    ROOM_REPLY_SAVED=4, ROOM_REPLY_UNCERTAIN=5, ROOM_REPLY_CANCELLED=6
} room_setup_reply_code;
typedef struct { uint64_t session, last_ms; bool seen; } room_setup_dispatch;
typedef bool (*room_setup_live)(void *ctx,uint64_t trial);
#define ROOM_SETUP_REPLY_SIZE 11u

/* Fixed response: version, result, state (0 idle/1 trial/2 verified/3 drain),
 * big-endian trial ID (zero unless active). No credentials or other data. */
static inline void room_setup_reply(uint8_t out[ROOM_SETUP_REPLY_SIZE],room_setup_reply_code code,
    const room_phone_setup *p) {
    uint64_t t=0;
    out[0]=1; out[1]=(uint8_t)code; out[2]=0;
    if (p->drain) out[2]=3;
    else if (p->window && p->enrollment.state!=ENROLL_IDLE) {
        out[2]=(uint8_t)p->enrollment.state; t=p->enrollment.generation;
    }
    for (size_t i=11;i>3;) { out[--i]=(uint8_t)t; t>>=8; }
}
/* True means one reply is owed to the original authenticated session, even
 * if the owner just closed it. Do not accept more requests after close;
 * transport must send that final reply or time out before tearing down AP.
 * False means no reply. Input/output must be disjoint; caller wipes input. */
static inline bool room_setup_handle(room_setup_dispatch *d,room_phone_setup *p,uint64_t now,
    uint64_t session,const uint8_t *in,size_t len,uint8_t out[ROOM_SETUP_REPLY_SIZE],
    const room_enroll_store *store,room_setup_live live,void *ctx) {
    if (!out) return false;
    memset(out,0,ROOM_SETUP_REPLY_SIZE);
    if (!d || !p) return false;
    phone_tick(p,now);
    if (!phone_authorized(p,session)) return false;
    if (d->session!=session) *d=(room_setup_dispatch){.session=session};
    if (d->seen && now<d->last_ms) { phone_close(p); return false; }
    room_setup_request r;
    bool valid=room_setup_parse(in,len,&r);
    if (valid && r.command==ROOM_SETUP_CANCEL) {
        phone_close(p); room_setup_reply(out,ROOM_REPLY_CANCELLED,p); return true;
    }
    if (d->seen && now-d->last_ms<100u) {
        room_setup_reply(out,ROOM_REPLY_WAIT,p); return true;
    }
    d->seen=true; d->last_ms=now;
    room_setup_reply_code code=ROOM_REPLY_REJECTED;
    if (valid) switch (r.command) {
    case ROOM_SETUP_STATUS: code=ROOM_REPLY_OK; break;
    case ROOM_SETUP_STAGE:
        if (phone_stage(p,now,session,store,in+r.ssid_offset,r.ssid_len,in+r.pass_offset,r.pass_len))
            code=ROOM_REPLY_OK;
        break;
    case ROOM_SETUP_SAVE:
        if (!p->drain && p->enrollment.state==ENROLL_VERIFIED && r.trial==p->enrollment.generation) {
            room_enroll_result result=phone_save(p,now,session,r.trial,live && live(ctx,r.trial),store);
            if (result==ENROLL_SAVED) code=ROOM_REPLY_SAVED;
            else if (result==ENROLL_STORAGE_UNCERTAIN) code=ROOM_REPLY_UNCERTAIN;
        }
        break;
    default: break;
    }
    room_setup_reply(out,code,p);
    return true;
}
#endif
