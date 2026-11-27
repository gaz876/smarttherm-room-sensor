#ifndef MOCK_FREERTOS_H
#define MOCK_FREERTOS_H
#include <stdint.h>
#define pdTRUE 1
#define pdPASS 1
#define portMAX_DELAY UINT32_MAX
#ifdef ROOM_ADMISSION_CS_TEST
#include <stdatomic.h>
#include <stdbool.h>
typedef struct { atomic_bool held; } portMUX_TYPE;
#define portMUX_INITIALIZE(m) atomic_init(&(m)->held,false)
static inline void taskENTER_CRITICAL(portMUX_TYPE *m) {
    while(atomic_exchange_explicit(&m->held,true,memory_order_acquire)){}
}
static inline void taskEXIT_CRITICAL(portMUX_TYPE *m) {
    atomic_store_explicit(&m->held,false,memory_order_release);
}
#endif
#endif
