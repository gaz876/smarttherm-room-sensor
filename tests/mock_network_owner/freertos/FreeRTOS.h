#ifndef MOCK_FREERTOS_H
#define MOCK_FREERTOS_H
typedef unsigned TickType_t;
#define pdPASS 1
#define pdMS_TO_TICKS(ms) ((TickType_t)((ms)/10u))
#endif
