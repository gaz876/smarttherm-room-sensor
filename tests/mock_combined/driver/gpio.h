#ifndef MOCK_COMBINED_GPIO_H
#define MOCK_COMBINED_GPIO_H
#include <stdint.h>
#define GPIO_MODE_INPUT 1
#define GPIO_PULLUP_ENABLE 1
#define GPIO_PULLDOWN_DISABLE 0
#define GPIO_INTR_DISABLE 0
#define GPIO_NUM_9 9
typedef struct { uint64_t pin_bit_mask; int mode,pull_up_en,pull_down_en,intr_type; } gpio_config_t;
int gpio_config(const gpio_config_t *);
int gpio_get_level(int);
#endif
