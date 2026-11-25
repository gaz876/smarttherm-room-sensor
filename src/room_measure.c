#include "room_measure.h"
#include <math.h>
void room_init(room_measure *m) {
    if (m) *m = (room_measure){.interval_ms=ROOM_DEFAULT_INTERVAL_MS,
        .reset_needed=true, .status=ROOM_NOT_READY};
}
bool room_configure(room_measure *m, uint32_t interval_ms, double correction) {
    if (!m || interval_ms < ROOM_MIN_INTERVAL_MS || interval_ms > ROOM_MAX_INTERVAL_MS ||
        !isfinite(correction) || fabs(correction)>10.0) return false;
    m->interval_ms=interval_ms;
    m->correction=correction;
    m->sample=(sht40_sample){0};
    m->attempted=false;
    m->status=ROOM_NOT_READY;
    return true;
}
static bool wait_checked(const room_bus *bus, uint32_t ms) {
    uint64_t before=bus->now_ms(bus->ctx);
    bus->wait_ms(bus->ctx,ms);
    uint64_t after=bus->now_ms(bus->ctx);
    return after>=before && after-before>=ms;
}
static void fail(room_measure *m, room_status status) {
    m->sample=(sht40_sample){0};
    m->status=status;
    m->reset_needed=true;
}
bool room_poll(room_measure *m, const room_bus *bus) {
    if (!m) return false;
    if (!bus || !bus->write || !bus->read6 || !bus->wait_ms || !bus->now_ms) {
        fail(m,ROOM_IO_ERROR);
        return false;
    }
    uint64_t now=bus->now_ms(bus->ctx);
    if (m->attempted && (now<m->last_start_ms ||
        (m->status==ROOM_OK && now<m->last_complete_ms))) {
        fail(m,ROOM_INVALID);
        m->attempted=false;
        return false;
    }
    if (m->attempted && now-m->last_start_ms<m->interval_ms) return false;
    m->attempted=true;
    m->last_start_ms=now;
    m->sample=(sht40_sample){0};
    m->status=ROOM_NOT_READY;
    if (m->reset_needed) {
        if (!bus->write(bus->ctx,0x94)) { fail(m,ROOM_IO_ERROR); return true; }
        if (!wait_checked(bus,2)) { fail(m,ROOM_INVALID); return true; }
        m->reset_needed=false;
    }
    if (!bus->write(bus->ctx,0xfd)) { fail(m,ROOM_IO_ERROR); return true; }
    if (!wait_checked(bus,10)) { fail(m,ROOM_INVALID); return true; }
    uint8_t frame[6]={0};
    if (!bus->read6(bus->ctx,frame,sizeof(frame))) { fail(m,ROOM_IO_ERROR); return true; }
    uint64_t end=bus->now_ms(bus->ctx);
    if (end<now || end-now>=m->interval_ms ||
        !sht40_decode(frame,sizeof(frame),m->correction,now,&m->sample)) {
        fail(m,ROOM_INVALID); return true;
    }
    m->last_complete_ms=end;
    m->status=ROOM_OK;
    return true;
}
room_status room_snapshot(const room_measure *m, uint64_t now, sht40_sample *out) {
    if (!out) return ROOM_INVALID;
    *out=(sht40_sample){0};
    if (!m) return ROOM_NOT_READY;
    if (m->status!=ROOM_OK) return m->status;
    if (now<m->last_complete_ms || now<m->last_start_ms || now-m->last_start_ms>=m->interval_ms ||
        !sht40_fresh(&m->sample,now,m->interval_ms)) return ROOM_STALE;
    *out=m->sample;
    return ROOM_OK;
}
