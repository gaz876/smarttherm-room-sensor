#ifndef MOCK_TASK_H
#define MOCK_TASK_H
#include "FreeRTOS.h"
typedef struct mock_task *TaskHandle_t;
TaskHandle_t xTaskGetCurrentTaskHandle(void);
int xTaskCreate(void (*fn)(void *),const char *,unsigned,void *,unsigned,TaskHandle_t *);
uint32_t ulTaskNotifyTake(int,uint32_t);
void xTaskNotifyGive(TaskHandle_t);
void vTaskDelay(uint32_t);
void vTaskDelete(TaskHandle_t);
#endif
