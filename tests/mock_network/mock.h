#ifndef MOCK_NETWORK_H
#define MOCK_NETWORK_H
#include <stddef.h>
#include <stdint.h>
typedef unsigned TickType_t;
#define pdPASS 1
#define pdMS_TO_TICKS(ms) ((ms)/10u)
#define ESP_LOGI(...) ((void)0)
int xTaskCreate(void (*fn)(void *),const char *,unsigned,void *,unsigned,void *);
void vTaskDelay(TickType_t);
void vTaskDelete(void *);
#endif
