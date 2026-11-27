#ifndef MOCK_EVENT_QUEUE_H
#define MOCK_EVENT_QUEUE_H
#include "FreeRTOS.h"
#include <stdint.h>
typedef void *QueueHandle_t;
typedef struct { unsigned placeholder; } StaticQueue_t;
QueueHandle_t xQueueCreateStatic(unsigned,unsigned,uint8_t *,StaticQueue_t *);
int xQueueSend(QueueHandle_t,const void *,unsigned);
int xQueueReceive(QueueHandle_t,void *,unsigned);
unsigned uxQueueMessagesWaiting(QueueHandle_t);
#endif
