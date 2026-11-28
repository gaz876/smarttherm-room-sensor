#ifndef MOCK_TASK_H
#define MOCK_TASK_H
#include "FreeRTOS.h"
TaskHandle_t xTaskGetCurrentTaskHandle(void);
int xTaskCreate(void (*)(void *),const char *,unsigned,void *,unsigned,TaskHandle_t *);
uint32_t ulTaskNotifyTake(int,uint32_t);
void xTaskNotifyGive(TaskHandle_t);
void vTaskDelay(TickType_t);
#endif
