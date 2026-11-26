#ifndef ROOM_WIFI_ENROLLMENT_H
#define ROOM_WIFI_ENROLLMENT_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Single owner, monotonic milliseconds. Trusted transport must authenticate
 * requests, bind events to the actual attempt, and prove fresh DHCP. This
 * module is not a network protocol or an encrypted storage implementation. */
typedef struct {
    uint8_t ssid_len, pass_len;
    uint8_t ssid[32], pass[63];
} room_enroll_profile;
typedef enum { ENROLL_IDLE, ENROLL_TRIAL, ENROLL_VERIFIED } room_enroll_state;
typedef enum { ENROLL_REJECTED, ENROLL_SAVED, ENROLL_STORAGE_UNCERTAIN } room_enroll_result;
typedef struct {
    uint64_t generation, deadline;
    room_enroll_state state;
    bool storage_uncertain;
    room_enroll_profile candidate;
} room_enroll;
typedef struct {
    void *ctx;
    /* Must check encryption/key readiness without generating keys or writing. */
    bool (*ready)(void *);
    /* Commit one protected record; false may still mean flash was modified. */
    bool (*write)(void *, const room_enroll_profile *);
    bool (*read)(void *, room_enroll_profile *);
} room_enroll_store;
static inline void enroll_wipe(void *p,size_t n) {
    volatile unsigned char *v=p;
    while (n--) *v++=0;
}
static inline void enroll_cancel(room_enroll *e) {
    enroll_wipe(&e->candidate,sizeof(e->candidate));
    e->state=ENROLL_IDLE; e->deadline=0;
}
static inline void enroll_tick(room_enroll *e,uint64_t now) {
    if (e->state!=ENROLL_IDLE && now>=e->deadline) enroll_cancel(e);
}
static inline bool enroll_ready(const room_enroll_store *s) {
    return s && s->ready && s->write && s->read && s->ready(s->ctx);
}
static inline bool enroll_profile_valid(const uint8_t *ssid,size_t n,const uint8_t *pass,size_t m) {
    if (!ssid || !pass || n<1 || n>32 || m<8 || m>63) return false;
    for (size_t i=0;i<n;i++) if (!ssid[i]) return false;
    for (size_t i=0;i<m;i++) if (pass[i]<32 || pass[i]>126) return false;
    return true;
}
/* Caller supplies authorization from the protected setup session, not user input.
 * The transport must already have stopped any previous trial before staging. */
static inline uint64_t enroll_stage(room_enroll *e,uint64_t now,uint64_t setup_deadline,
    bool authorized,const room_enroll_store *s,const uint8_t *ssid,size_t n,const uint8_t *pass,size_t m) {
    enroll_tick(e,now);
    if (e->storage_uncertain || e->state!=ENROLL_IDLE || !authorized || now>=setup_deadline ||
        e->generation==UINT64_MAX || !enroll_profile_valid(ssid,n,pass,m) || !enroll_ready(s)) return 0;
    uint64_t limit=now>UINT64_MAX-30000u?UINT64_MAX:now+30000u;
    e->deadline=limit<setup_deadline?limit:setup_deadline;
    e->candidate.ssid_len=(uint8_t)n; e->candidate.pass_len=(uint8_t)m;
    memcpy(e->candidate.ssid,ssid,n); memcpy(e->candidate.pass,pass,m);
    e->state=ENROLL_TRIAL;
    return ++e->generation;
}
static inline bool enroll_observe(room_enroll *e,uint64_t now,uint64_t token,bool fresh_link_and_dhcp) {
    enroll_tick(e,now);
    if (!token || token!=e->generation || e->state==ENROLL_IDLE) return false;
    if (!fresh_link_and_dhcp) { enroll_cancel(e); return false; }
    e->state=ENROLL_VERIFIED; return true;
}
/* A second live observation is required at explicit authenticated commit.
 * Never report success after an ambiguous write/readback; caller must quarantine
 * uncertain persistence and recover explicitly. Never auto-erase or retry writes. */
static inline room_enroll_result enroll_commit(room_enroll *e,uint64_t now,uint64_t token,
    bool authorized,bool live,const room_enroll_store *s) {
    enroll_tick(e,now);
    if (!token || token!=e->generation || !authorized || e->state!=ENROLL_VERIFIED) return ENROLL_REJECTED;
    if (!live || !enroll_ready(s)) { enroll_cancel(e); return ENROLL_REJECTED; }
    room_enroll_profile readback={0};
    bool ok=s->write(s->ctx,&e->candidate) && s->read(s->ctx,&readback) &&
        readback.ssid_len==e->candidate.ssid_len && readback.pass_len==e->candidate.pass_len &&
        memcmp(readback.ssid,e->candidate.ssid,sizeof(readback.ssid))==0 &&
        memcmp(readback.pass,e->candidate.pass,sizeof(readback.pass))==0;
    enroll_wipe(&readback,sizeof(readback)); enroll_cancel(e);
    if (!ok) e->storage_uncertain=true;
    return ok?ENROLL_SAVED:ENROLL_STORAGE_UNCERTAIN;
}
#endif
