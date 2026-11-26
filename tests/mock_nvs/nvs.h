#ifndef MOCK_NVS_H
#define MOCK_NVS_H
#include <stddef.h>
#include <stdint.h>
typedef int esp_err_t;
typedef uint32_t nvs_handle_t;
typedef enum { NVS_READONLY, NVS_READWRITE } nvs_open_mode_t;
#define ESP_OK 0
#define ESP_ERR_NVS_NOT_FOUND 1
#define ESP_ERR_NVS_TYPE_MISMATCH 2
#define ESP_ERR_NVS_INVALID_LENGTH 3
esp_err_t nvs_open(const char *, nvs_open_mode_t, nvs_handle_t *);
esp_err_t nvs_get_blob(nvs_handle_t,const char *,void *,size_t *);
esp_err_t nvs_set_blob(nvs_handle_t,const char *,const void *,size_t);
esp_err_t nvs_commit(nvs_handle_t);
void nvs_close(nvs_handle_t);
#endif
