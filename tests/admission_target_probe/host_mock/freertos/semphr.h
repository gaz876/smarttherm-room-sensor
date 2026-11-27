#ifndef MOCK_SEM_H
#define MOCK_SEM_H
#include "FreeRTOS.h"
typedef sem_t *SemaphoreHandle_t;
SemaphoreHandle_t xSemaphoreCreateBinary(void);
int xSemaphoreTake(SemaphoreHandle_t,uint32_t);
int xSemaphoreGive(SemaphoreHandle_t);
#endif
