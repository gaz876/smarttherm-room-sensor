#ifndef ROOM_SETTINGS_NVS_H
#define ROOM_SETTINGS_NVS_H
#include "room_settings.h"
#include "nvs.h"
#include "nvs_flash.h"

/* One namespace/key. Never erase an entire NVS partition on initialization failure. */
static inline room_settings_result room_nvs_read(void *ctx, uint8_t *data, size_t *len) {
    (void)ctx;
    nvs_handle_t h;
    esp_err_t e=nvs_open("room_measure",NVS_READONLY,&h);
    if (e==ESP_ERR_NVS_NOT_FOUND) return ROOM_SETTINGS_MISSING;
    if (e!=ESP_OK) return ROOM_SETTINGS_IO_ERROR;
    e=nvs_get_blob(h,"settings",data,len);
    nvs_close(h);
    if (e==ESP_ERR_NVS_NOT_FOUND) return ROOM_SETTINGS_MISSING;
    if (e==ESP_ERR_NVS_TYPE_MISMATCH || e==ESP_ERR_NVS_INVALID_LENGTH) return ROOM_SETTINGS_UNSUPPORTED;
    return e==ESP_OK ? ROOM_SETTINGS_OK : ROOM_SETTINGS_IO_ERROR;
}
static inline bool room_nvs_write(void *ctx, const uint8_t *data, size_t len) {
    (void)ctx;
    nvs_handle_t h;
    if (nvs_open("room_measure",NVS_READWRITE,&h)!=ESP_OK) return false;
    esp_err_t e=nvs_set_blob(h,"settings",data,len);
    if (e==ESP_OK) e=nvs_commit(h);
    nvs_close(h);
    return e==ESP_OK;
}
static inline room_settings_store room_nvs_store(void) {
    return (room_settings_store){NULL,room_nvs_read,room_nvs_write};
}
#endif
