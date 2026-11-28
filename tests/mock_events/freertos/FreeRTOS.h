#ifndef MOCK_EVENT_FREERTOS_H
#define MOCK_EVENT_FREERTOS_H
#include <assert.h>
/* These older fixtures run in one host thread. The new peer test uses a
 * pthread mutex instead and is not built with this mock. */
typedef unsigned portMUX_TYPE;
#define portMUX_INITIALIZE(p) (*(p)=0u)
#define taskENTER_CRITICAL(p) do { assert(*(p)==0u); *(p)=1u; } while(0)
#define taskEXIT_CRITICAL(p) do { assert(*(p)==1u); *(p)=0u; } while(0)
#define pdPASS 1
#endif
