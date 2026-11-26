#ifndef ROOM_PHONE_SETUP_H
#define ROOM_PHONE_SETUP_H
#include "room_wifi_enrollment.h"
/* Single network owner. All authorization/session IDs originate from the
 * trusted transport, never a request field. No transport or crypto here. */
typedef enum { PHONE_PROFILE_MISSING, PHONE_PROFILE_VALID, PHONE_PROFILE_ERROR } phone_profile_state;
typedef struct {
    room_enroll enrollment;
    uint64_t deadline, session, last_session, saved_generation;
    bool window, first_used, drain;
} room_phone_setup;
static inline void phone_cancel_trial(room_phone_setup *p) {
    if (p->enrollment.state!=ENROLL_IDLE) p->drain=true;
    enroll_cancel(&p->enrollment);
}
static inline void phone_close(room_phone_setup *p) {
    phone_cancel_trial(p); p->window=false; p->session=0; p->deadline=0;
}
static inline void phone_tick(room_phone_setup *p,uint64_t now) {
    if (!p->window) return;
    if (now>=p->deadline) { phone_close(p); return; }
    if (p->enrollment.state!=ENROLL_IDLE && now>=p->enrollment.deadline) phone_cancel_trial(p);
}
/* Drain acknowledgement is issued only by owner after actual station cleanup. */
static inline void phone_drained(room_phone_setup *p) { p->drain=false; }
static inline bool phone_open(room_phone_setup *p,uint64_t now,phone_profile_state profile,
    bool physical_boot,bool identity_ready,const room_enroll_store *store) {
    phone_tick(p,now);
    if (p->window || p->drain || p->enrollment.storage_uncertain || !identity_ready ||
        !enroll_ready(store) || profile==PHONE_PROFILE_ERROR ||
        (profile!=PHONE_PROFILE_VALID && profile!=PHONE_PROFILE_MISSING) ||
        (!physical_boot && (profile!=PHONE_PROFILE_MISSING || p->first_used))) return false;
    p->first_used=true; p->window=true;
    p->deadline=now>UINT64_MAX-300000u?UINT64_MAX:now+300000u;
    return true;
}
/* Monotonic per-boot transport generation; never use HTTP cookie as proof. */
static inline bool phone_claim(room_phone_setup *p,uint64_t now,uint64_t authenticated_session) {
    phone_tick(p,now);
    if (!p->window || p->drain || p->session || authenticated_session<=p->last_session) return false;
    p->session=authenticated_session; p->last_session=authenticated_session; return true;
}
static inline bool phone_authorized(const room_phone_setup *p,uint64_t session) {
    return p->window && session && session==p->session;
}
static inline uint64_t phone_stage(room_phone_setup *p,uint64_t now,uint64_t session,
    const room_enroll_store *store,const uint8_t *ssid,size_t n,const uint8_t *pass,size_t m) {
    phone_tick(p,now);
    if (!phone_authorized(p,session)||p->drain) return 0;
    return enroll_stage(&p->enrollment,now,p->deadline,true,store,ssid,n,pass,m);
}
/* Owner supplies observed result for the actual station attempt. */
static inline bool phone_trial_result(room_phone_setup *p,uint64_t now,uint64_t token,bool connected) {
    phone_tick(p,now);
    if (!p->window || !token || token!=p->enrollment.generation || p->enrollment.state==ENROLL_IDLE) return false;
    if (!connected) { phone_cancel_trial(p); return false; }
    return enroll_observe(&p->enrollment,now,token,true);
}
static inline room_enroll_result phone_save(room_phone_setup *p,uint64_t now,uint64_t session,
    uint64_t token,bool live,const room_enroll_store *store) {
    phone_tick(p,now);
    if (!phone_authorized(p,session)||p->drain||p->enrollment.state!=ENROLL_VERIFIED||
        !token||token!=p->enrollment.generation) return ENROLL_REJECTED;
    room_enroll_result result=enroll_commit(&p->enrollment,now,token,true,live,store);
    /* Persistent per-boot receipt of the authenticated, verified commit.
     * Independent of the final reply: delivery failure cannot undo flash. */
    if (result==ENROLL_SAVED) p->saved_generation=token;
    if (p->enrollment.state==ENROLL_IDLE) p->drain=true;
    if (result!=ENROLL_REJECTED) phone_close(p);
    return result;
}
static inline void phone_session_lost(room_phone_setup *p,uint64_t session) {
    if (phone_authorized(p,session)) phone_close(p);
}
#endif
