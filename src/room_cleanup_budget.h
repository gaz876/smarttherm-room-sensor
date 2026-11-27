#ifndef ROOM_CLEANUP_BUDGET_H
#define ROOM_CLEANUP_BUDGET_H
#include <stdbool.h>
#include <stdint.h>

/* Owner-only absolute cleanup budget. UINT64_MAX is an invalid clock sample.
 * The caller must latch failure and not reset until a fully drained window. */
static inline uint64_t room_cleanup_limit(uint64_t now) {
    if (now==UINT64_MAX) return 0;
    return now>UINT64_MAX-5000u?UINT64_MAX:now+5000u;
}
static inline bool room_cleanup_check(uint64_t *fixed,uint64_t *last,uint64_t now,uint64_t deadline) {
    if (!fixed || !last || !deadline || now>=deadline || now<*last ||
        deadline-now>5000u || (*fixed && *fixed!=deadline)) return false;
    *fixed=deadline; *last=now; return true;
}
#endif
