#ifndef MOCK_ESP_HEAP_CAPS_H
#define MOCK_ESP_HEAP_CAPS_H
#include <stddef.h>
#include <stdint.h>
#define MALLOC_CAP_INTERNAL 1u
#define MALLOC_CAP_8BIT 2u
size_t heap_caps_get_free_size(uint32_t);
size_t heap_caps_get_minimum_free_size(uint32_t);
size_t heap_caps_get_largest_free_block(uint32_t);
#endif
