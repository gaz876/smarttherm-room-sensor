#ifndef ROOM_SETTINGS_H
#define ROOM_SETTINGS_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ROOM_SETTINGS_BYTES 12u
#define ROOM_SETTINGS_MIN_MS 1000u
#define ROOM_SETTINGS_MAX_MS 3600000u
#define ROOM_SETTINGS_MAX_CORRECTION_MDEG 10000

typedef struct {
    uint32_t interval_ms;
    int32_t correction_mdeg;
} room_settings;

typedef enum {
    ROOM_SETTINGS_OK, ROOM_SETTINGS_MISSING, ROOM_SETTINGS_INVALID,
    ROOM_SETTINGS_IO_ERROR, ROOM_SETTINGS_UNCHANGED, ROOM_SETTINGS_UNSUPPORTED
} room_settings_result;

typedef struct {
    void *ctx;
    room_settings_result (*read)(void *, uint8_t *, size_t *);
    bool (*write_commit)(void *, const uint8_t *, size_t);
} room_settings_store;

static inline bool room_settings_valid(room_settings s) {
    return s.interval_ms >= ROOM_SETTINGS_MIN_MS && s.interval_ms <= ROOM_SETTINGS_MAX_MS &&
        s.correction_mdeg >= -ROOM_SETTINGS_MAX_CORRECTION_MDEG &&
        s.correction_mdeg <= ROOM_SETTINGS_MAX_CORRECTION_MDEG;
}
static inline void room_settings_put32(uint8_t *p, uint32_t v) {
    for (unsigned i=0; i<4; ++i) p[i]=(uint8_t)(v>>(8u*i));
}
static inline uint32_t room_settings_get32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24);
}
/* Stable wire format: RS, version, reserved, interval ms, offset + 10000 mdeg. NVS checks its blob CRC. */
static inline bool room_settings_encode(room_settings s, uint8_t out[ROOM_SETTINGS_BYTES]) {
    if (!out || !room_settings_valid(s)) return false;
    out[0]='R'; out[1]='S'; out[2]=1; out[3]=0;
    room_settings_put32(out+4,s.interval_ms);
    room_settings_put32(out+8,(uint32_t)(s.correction_mdeg+ROOM_SETTINGS_MAX_CORRECTION_MDEG));
    return true;
}
static inline room_settings_result room_settings_decode(const uint8_t *p, size_t n, room_settings *out) {
    if (!p || !out || n<3 || p[0]!='R' || p[1]!='S') return ROOM_SETTINGS_INVALID;
    if (p[2]!=1) return ROOM_SETTINGS_UNSUPPORTED;
    if (n!=ROOM_SETTINGS_BYTES || p[3]!=0) return ROOM_SETTINGS_INVALID;
    uint32_t offset=room_settings_get32(p+8);
    if (offset>2u*ROOM_SETTINGS_MAX_CORRECTION_MDEG) return ROOM_SETTINGS_INVALID;
    room_settings s={room_settings_get32(p+4),(int32_t)offset-ROOM_SETTINGS_MAX_CORRECTION_MDEG};
    if (!room_settings_valid(s)) return ROOM_SETTINGS_INVALID;
    *out=s;
    return ROOM_SETTINGS_OK;
}
/* Read never writes; defaults are returned on failure, status remains visible. */
static inline room_settings_result room_settings_load(const room_settings_store *store, room_settings defaults, room_settings *out) {
    if (!out || !room_settings_valid(defaults)) return ROOM_SETTINGS_INVALID;
    *out=defaults;
    if (!store || !store->read) return ROOM_SETTINGS_IO_ERROR;
    uint8_t data[ROOM_SETTINGS_BYTES]={0}; size_t len=sizeof(data);
    room_settings_result r=store->read(store->ctx,data,&len);
    if (r!=ROOM_SETTINGS_OK) return r;
    if (len>sizeof(data)) return ROOM_SETTINGS_UNSUPPORTED;
    return room_settings_decode(data,len,out);
}
/* Single owner. Only explicit save/reset writes. Preserve unknown formats unless explicit reset. */
static inline room_settings_result room_settings_save(const room_settings_store *store, room_settings s, bool reset) {
    uint8_t data[ROOM_SETTINGS_BYTES];
    if (!room_settings_encode(s,data)) return ROOM_SETTINGS_INVALID;
    if (!store || !store->read || !store->write_commit) return ROOM_SETTINGS_IO_ERROR;
    room_settings old;
    room_settings_result r=room_settings_load(store,s,&old);
    if (r==ROOM_SETTINGS_OK && old.interval_ms==s.interval_ms && old.correction_mdeg==s.correction_mdeg)
        return ROOM_SETTINGS_UNCHANGED;
    if (r==ROOM_SETTINGS_UNSUPPORTED && !reset) return r;
    if (r!=ROOM_SETTINGS_OK && r!=ROOM_SETTINGS_MISSING && r!=ROOM_SETTINGS_INVALID && r!=ROOM_SETTINGS_UNSUPPORTED)
        return ROOM_SETTINGS_IO_ERROR;
    return store->write_commit(store->ctx,data,sizeof(data)) ? ROOM_SETTINGS_OK : ROOM_SETTINGS_IO_ERROR;
}
#endif
