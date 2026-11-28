#ifndef MOCK_FREERTOS_H
#define MOCK_FREERTOS_H
#include <pthread.h>
#include <assert.h>
typedef pthread_mutex_t portMUX_TYPE;
#define portMUX_INITIALIZE(p) assert(pthread_mutex_init((p),NULL)==0)
#define taskENTER_CRITICAL(p) assert(pthread_mutex_lock(p)==0)
#define taskEXIT_CRITICAL(p) assert(pthread_mutex_unlock(p)==0)
#define pdPASS 1
#endif
