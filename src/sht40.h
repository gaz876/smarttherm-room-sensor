#ifndef SHT40_H
#define SHT40_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
typedef struct { double temperature, humidity; uint64_t measured_ms; bool valid; } sht40_sample;
uint8_t sht40_crc(uint8_t a, uint8_t b);
bool sht40_decode(const uint8_t *frame, size_t len, double dt, double dh, uint64_t now, sht40_sample *out);
bool sht40_fresh(const sht40_sample *s, uint64_t now, uint64_t max_age);
#endif
