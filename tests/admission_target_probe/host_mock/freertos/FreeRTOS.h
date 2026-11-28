#ifndef PROBE_MOCK_RTOS_H
#define PROBE_MOCK_RTOS_H
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>
typedef pthread_mutex_t portMUX_TYPE;
#define portMUX_INITIALIZE(m) ((void)pthread_mutex_init((m),NULL))
#define taskENTER_CRITICAL(m) ((void)pthread_mutex_lock(m))
#define taskEXIT_CRITICAL(m) ((void)pthread_mutex_unlock(m))
#define pdTRUE 1
#define pdPASS 1
#define portMAX_DELAY UINT32_MAX
#define pdMS_TO_TICKS(ms) (ms)
#endif
