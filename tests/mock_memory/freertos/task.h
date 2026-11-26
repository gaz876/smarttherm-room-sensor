#ifndef MOCK_TASK_H
#define MOCK_TASK_H
#include "FreeRTOS.h"
typedef void *TaskHandle_t;
UBaseType_t uxTaskGetStackHighWaterMark(TaskHandle_t);
#endif
