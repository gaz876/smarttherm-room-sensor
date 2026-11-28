#ifndef MOCK_EVENT_TASK_H
#define MOCK_EVENT_TASK_H
#include "FreeRTOS.h"
typedef void *TaskHandle_t;
TaskHandle_t xTaskGetCurrentTaskHandle(void);
#endif
