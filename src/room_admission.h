#ifndef ROOM_ADMISSION_H
#define ROOM_ADMISSION_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <stdatomic.h>

/* Offline contract; a boot-lifetime gate is initialized once before publication.
 * A gate never reopens. Callers retain the context even after denied entry until
 * their source of callbacks is stopped. A lease is exclusive, not copyable;
 * transfer to another task requires an external synchronized handoff. Release
 * only after the last access to the protected context and all owned buffers. */
_Static_assert(UINT_MAX == UINT32_MAX, "requires 32-bit unsigned");
_Static_assert(ATOMIC_INT_LOCK_FREE == 2, "requires lock-free unsigned atomics");
#define ROOM_ADMISSION_CLOSED (UINT32_C(1)<<31)
#define ROOM_ADMISSION_MAX 32u
typedef struct { uint64_t window; atomic_uint state; } room_admission;
typedef struct { room_admission *gate; } room_lease;
static inline bool room_admission_init(room_admission *g,uint64_t window) {
    if(!g||!window)return false;
    g->window=window;atomic_init(&g->state,0u);return true;
}
static inline bool room_admission_enter(room_admission *g,uint64_t window,room_lease *lease) {
    if(!g||!lease||lease->gate||!window||window!=g->window)return false;
    unsigned state=atomic_load_explicit(&g->state,memory_order_acquire);
    for(unsigned attempt=0;attempt<32;++attempt) {
        if((state&ROOM_ADMISSION_CLOSED)||state>=ROOM_ADMISSION_MAX)return false;
        if(atomic_compare_exchange_weak_explicit(&g->state,&state,state+1u,memory_order_acq_rel,memory_order_acquire)) {
            lease->gate=g;return true;
        }
    }
    return false; /* bounded admission; contention may reject work */
}
static inline bool room_admission_leave(room_lease *lease) {
    if(!lease||!lease->gate)return false;
    room_admission *g=lease->gate;lease->gate=NULL;
    atomic_fetch_sub_explicit(&g->state,1u,memory_order_acq_rel);return true;
}
static inline bool room_admission_close(room_admission *g,uint64_t window) {
    if(!g||!window||window!=g->window)return false;
    atomic_fetch_or_explicit(&g->state,ROOM_ADMISSION_CLOSED,memory_order_acq_rel);return true;
}
static inline bool room_admission_drained(const room_admission *g,uint64_t window) {
    return g&&window&&window==g->window&&atomic_load_explicit(&g->state,memory_order_acquire)==ROOM_ADMISSION_CLOSED;
}
#endif
