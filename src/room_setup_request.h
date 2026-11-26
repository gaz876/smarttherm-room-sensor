#ifndef ROOM_SETUP_REQUEST_H
#define ROOM_SETUP_REQUEST_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Experimental application payload only, AFTER authentication/decryption.
 * Never treat parsing as authorization. No transport, persistence or crypto.
 * Caller owns an immutable input buffer and a separate output object; offsets
 * are valid only while that input is alive. Wipe the input after dispatch.
 * Semantic profile validation still belongs to the enrollment owner. */
typedef enum {
    ROOM_SETUP_STATUS=1, ROOM_SETUP_STAGE=2,
    ROOM_SETUP_SAVE=3, ROOM_SETUP_CANCEL=4
} room_setup_command;
typedef struct {
    room_setup_command command;
    uint64_t trial;
    size_t ssid_offset, ssid_len, pass_offset, pass_len;
} room_setup_request;

static inline bool room_setup_parse(const uint8_t *in, size_t len, room_setup_request *out) {
    if (!out) return false;
    *out=(room_setup_request){0};
    if (!in || len<2 || len>99 || in[0]!=1) return false;
    room_setup_request r={0};
    switch (in[1]) {
    case ROOM_SETUP_STATUS:
    case ROOM_SETUP_CANCEL:
        if (len!=2) return false;
        break;
    case ROOM_SETUP_STAGE:
        if (len<4) return false;
        r.ssid_len=in[2]; r.pass_len=in[3];
        if (r.ssid_len<1 || r.ssid_len>32 || r.pass_len<8 || r.pass_len>63 ||
            len!=4+r.ssid_len+r.pass_len) return false;
        r.ssid_offset=4; r.pass_offset=4+r.ssid_len;
        break;
    case ROOM_SETUP_SAVE:
        if (len!=10) return false;
        for (size_t i=2;i<10;i++) r.trial=(r.trial<<8)|in[i];
        if (!r.trial) return false;
        break;
    default: return false;
    }
    r.command=(room_setup_command)in[1];
    *out=r;
    return true;
}
#endif
