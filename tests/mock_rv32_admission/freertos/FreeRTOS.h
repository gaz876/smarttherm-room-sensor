#ifndef PROBE_FREERTOS_H
#define PROBE_FREERTOS_H
typedef struct { unsigned placeholder; } portMUX_TYPE;
void portMUX_INITIALIZE(portMUX_TYPE *);
void taskENTER_CRITICAL(portMUX_TYPE *);
void taskEXIT_CRITICAL(portMUX_TYPE *);
#endif
