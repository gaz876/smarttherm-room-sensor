#ifndef ROOM_I2C_RETRY_H
#define ROOM_I2C_RETRY_H
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    void *ctx;
    bool (*create_bus)(void *);
    bool (*add_device)(void *);
    void (*reset_bus)(void *);
    bool (*remove_device)(void *);
    bool (*delete_bus)(void *);
} room_i2c_ops;

typedef struct {
    bool bus, device, ready;
    unsigned io_failures;
    uint32_t delay_ms;
} room_i2c_retry;

static inline uint32_t room_i2c_backoff(room_i2c_retry *s) {
    uint32_t delay = s->delay_ms ? s->delay_ms : 1000u;
    s->delay_ms = delay >= 15000u ? 30000u : delay * 2u;
    return delay;
}

/* Single owner. Zero means ready. On failure retain ownership until cleanup succeeds. */
static inline uint32_t room_i2c_prepare(room_i2c_retry *s, const room_i2c_ops *o) {
    if (s->ready) return 0;
    if (s->device) {
        o->reset_bus(o->ctx);
        if (!o->remove_device(o->ctx)) return room_i2c_backoff(s);
        s->device = false;
    }
    if (s->bus) {
        if (!o->delete_bus(o->ctx)) return room_i2c_backoff(s);
        s->bus = false;
    }
    if (!o->create_bus(o->ctx)) return room_i2c_backoff(s);
    s->bus = true;
    if (!o->add_device(o->ctx)) return room_i2c_backoff(s);
    s->device = true;
    s->ready = true;
    s->io_failures = 0;
    return 0;
}

/* Only a valid measurement clears retry backoff. CRC/timing errors are not I/O errors. */
static inline uint32_t room_i2c_observe(room_i2c_retry *s, bool valid, bool io_error) {
    if (!s->ready) return 0;
    if (valid) s->delay_ms = 0;
    if (!io_error || valid) { s->io_failures = 0; return 0; }
    if (++s->io_failures < 3u) return 0;
    s->ready = false;
    return room_i2c_backoff(s);
}
#endif
