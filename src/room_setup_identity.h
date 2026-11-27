#ifndef ROOM_SETUP_IDENTITY_H
#define ROOM_SETUP_IDENTITY_H
#include "room_wifi_enrollment.h"

/* Versioned, externally provisioned, read-only identity record. Fixed
 * byte format, not a persisted C struct. No built-in/default credentials.
 * Syntax validation does not prove randomness, uniqueness or authenticity. */
enum { ROOM_ID_SALT_MAX=64, ROOM_ID_VERIFIER_SIZE=384,
    ROOM_ID_RECORD_SIZE=4+32+63+ROOM_ID_SALT_MAX+ROOM_ID_VERIFIER_SIZE };
typedef struct {
    uint8_t ssid[32],key[63],salt[ROOM_ID_SALT_MAX],verifier[ROOM_ID_VERIFIER_SIZE];
    uint8_t ssid_len,key_len,salt_len;
} room_setup_identity;
static inline bool room_id_zero(const uint8_t *p,size_t n) {
    uint8_t accum=0;
    for(size_t i=0;i<n;i++) accum|=p[i];
    return accum==0;
}
static inline bool room_id_decode(room_setup_identity *out,const uint8_t *record,size_t n) {
    if(!out)return false;
    enroll_wipe(out,sizeof(*out));
    if(!record||n!=ROOM_ID_RECORD_SIZE||record[0]!=1u||
        record[1]<1||record[1]>32||record[2]<16||record[2]>63||
        record[3]<8||record[3]>ROOM_ID_SALT_MAX)return false;
    size_t sn=record[1],kn=record[2],ln=record[3];
    const uint8_t *ssid=record+4,*key=ssid+32,*salt=key+63,*verifier=salt+ROOM_ID_SALT_MAX;
    if(!enroll_profile_valid(ssid,sn,key,kn)||
        !room_id_zero(ssid+sn,32-sn)||!room_id_zero(key+kn,63-kn)||
        !room_id_zero(salt+ln,ROOM_ID_SALT_MAX-ln)||
        room_id_zero(salt,ln)||room_id_zero(verifier,ROOM_ID_VERIFIER_SIZE))return false;
    out->ssid_len=(uint8_t)sn;out->key_len=(uint8_t)kn;out->salt_len=(uint8_t)ln;
    memcpy(out->ssid,ssid,sn);memcpy(out->key,key,kn);
    memcpy(out->salt,salt,ln);memcpy(out->verifier,verifier,ROOM_ID_VERIFIER_SIZE);
    return true;
}
#endif
