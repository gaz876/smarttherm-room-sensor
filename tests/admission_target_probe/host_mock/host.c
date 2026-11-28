#define _POSIX_C_SOURCE 200809L
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include "esp_timer.h"
struct mock_task { pthread_t thread; sem_t notify; void (*fn)(void *); void *ctx; };
static _Thread_local TaskHandle_t self;
static struct mock_task owner;
TaskHandle_t xTaskGetCurrentTaskHandle(void){return self;}
static void *run(void *v){self=v;self->fn(self->ctx);return NULL;}
int xTaskCreate(void (*fn)(void *),const char *name,unsigned stack,void *ctx,unsigned priority,TaskHandle_t *out){
    (void)name;(void)stack;(void)priority;TaskHandle_t t=calloc(1,sizeof *t);assert(t);assert(!sem_init(&t->notify,0,0));t->fn=fn;t->ctx=ctx;
    if(out){*out=t;}assert(!pthread_create(&t->thread,NULL,run,t));assert(!pthread_detach(t->thread));return pdPASS;
}
uint32_t ulTaskNotifyTake(int clear,uint32_t wait){assert(clear==pdTRUE&&wait==portMAX_DELAY);int rc;do{rc=sem_wait(&self->notify);}while(rc&&errno==EINTR);assert(!rc);return 1;}
void xTaskNotifyGive(TaskHandle_t t){assert(!sem_post(&t->notify));}
void vTaskDelay(uint32_t t){struct timespec ts={t/1000,(long)(t%1000)*1000000L};while(nanosleep(&ts,&ts)&&errno==EINTR){}}
void vTaskDelete(TaskHandle_t t){assert(!t);pthread_exit(NULL);}
SemaphoreHandle_t xSemaphoreCreateBinary(void){sem_t *s=malloc(sizeof *s);assert(s);assert(!sem_init(s,0,0));return s;}
int xSemaphoreTake(SemaphoreHandle_t s,uint32_t ms){struct timespec ts;assert(!clock_gettime(CLOCK_REALTIME,&ts));ts.tv_sec+=ms/1000;ts.tv_nsec+=(long)(ms%1000)*1000000L;ts.tv_sec+=ts.tv_nsec/1000000000L;ts.tv_nsec%=1000000000L;int r;do{r=sem_timedwait(s,&ts);}while(r&&errno==EINTR);return r?0:pdTRUE;}
int xSemaphoreGive(SemaphoreHandle_t s){return sem_post(s)?0:pdTRUE;}
int64_t esp_timer_get_time(void){struct timespec ts;assert(!clock_gettime(CLOCK_MONOTONIC,&ts));return (int64_t)ts.tv_sec*1000000+ts.tv_nsec/1000;}
void app_main(void);
int main(void){self=&owner;app_main();return 0;}
