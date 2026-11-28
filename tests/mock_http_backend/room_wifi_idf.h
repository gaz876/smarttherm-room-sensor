#ifndef ROOM_WIFI_IDF_H
#define ROOM_WIFI_IDF_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
bool room_wifi_idf_init(const uint8_t *,size_t,const uint8_t *,size_t);
bool room_wifi_idf_step(bool *);
#endif
