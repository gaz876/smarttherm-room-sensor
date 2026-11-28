#ifndef MOCK_FREERTOS_H
#define MOCK_FREERTOS_H
#include <stddef.h>
#include <stdint.h>
typedef void *TaskHandle_t;
typedef unsigned TickType_t;
#define pdMS_TO_TICKS(ms) ((TickType_t)((ms)/10u))
#define pdTRUE 1
#define pdPASS 1
#define portMAX_DELAY UINT32_MAX
#endif
