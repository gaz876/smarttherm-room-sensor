#ifndef MOCK_EVENT_QUEUE_H
#define MOCK_EVENT_QUEUE_H
#include "FreeRTOS.h"
typedef void *QueueHandle_t;
int xQueueSend(QueueHandle_t,const void *,unsigned);
int xQueueReceive(QueueHandle_t,void *,unsigned);
unsigned uxQueueMessagesWaiting(QueueHandle_t);
#endif
