#ifndef ROOM_MEASURE_H
#define ROOM_MEASURE_H
#include "sht40.h"
#define ROOM_DEFAULT_INTERVAL_MS 10000u
#define ROOM_MIN_INTERVAL_MS 1000u
#define ROOM_MAX_INTERVAL_MS 3600000u
/* Single owner only. A future concurrent transport must use a synchronized snapshot. */
typedef enum { ROOM_NOT_READY, ROOM_OK, ROOM_IO_ERROR, ROOM_INVALID, ROOM_STALE } room_status;
typedef struct {
    void *ctx;
    bool (*write)(void *, uint8_t);
    bool (*read6)(void *, uint8_t *, size_t);
    void (*wait_ms)(void *, uint32_t);
    uint64_t (*now_ms)(void *);
} room_bus;
typedef struct {
    sht40_sample sample;
    uint32_t interval_ms;
    double correction;
    uint64_t last_start_ms, last_complete_ms;
    bool attempted, reset_needed;
    room_status status;
} room_measure;
void room_init(room_measure *m);
bool room_configure(room_measure *m, uint32_t interval_ms, double correction);
/* Returns true when a measurement attempt was made, not necessarily successful. */
bool room_poll(room_measure *m, const room_bus *bus);
/* No numeric output on failure. Freshness expires at the next scheduled attempt. */
room_status room_snapshot(const room_measure *m, uint64_t now_ms, sht40_sample *out);
#endif
