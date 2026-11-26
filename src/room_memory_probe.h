#ifndef ROOM_MEMORY_PROBE_H
#define ROOM_MEMORY_PROBE_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
/* Diagnostic snapshot, called from task context only. No allocation, logs,
 * credentials or automatic thresholds. Values are bytes on ESP-IDF. The
 * minimum heap is the SDK low-water mark, not a per-operation measurement. */
typedef struct {
    size_t free_internal, minimum_internal, largest_internal, task_stack_min;
} room_memory_snapshot;
static inline bool room_memory_sample(room_memory_snapshot *out) {
    if (!out) return false;
    const uint32_t caps=MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT;
    *out=(room_memory_snapshot){
        .free_internal=heap_caps_get_free_size(caps),
        .minimum_internal=heap_caps_get_minimum_free_size(caps),
        .largest_internal=heap_caps_get_largest_free_block(caps),
        .task_stack_min=(size_t)uxTaskGetStackHighWaterMark(NULL)
    };
    return true;
}
#endif
