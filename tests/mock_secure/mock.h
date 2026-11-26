#ifndef MOCK_SECURE_H
#define MOCK_SECURE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_ERR_NVS_NOT_FOUND 1
#define ESP_PARTITION_TYPE_DATA 1
#define ESP_PARTITION_SUBTYPE_DATA_NVS 2
#define NVS_READONLY 0
#define NVS_READWRITE 1
#define EFUSE_BLK_KEY0 4
#define ESP_EFUSE_KEY_PURPOSE_HMAC_UP 8
typedef int esp_efuse_block_t;
typedef struct { int id; } esp_efuse_desc_t;
#define F(n) extern const esp_efuse_desc_t *ESP_EFUSE_RD_DIS_KEY##n[]; extern const esp_efuse_desc_t *ESP_EFUSE_WR_DIS_KEY##n[];
F(0) F(1) F(2) F(3) F(4) F(5)
#undef F
typedef struct { bool encrypted,readonly; size_t size; } esp_partition_t;
typedef unsigned nvs_handle_t;
typedef struct { int id; } nvs_sec_scheme_t;
typedef int hmac_key_id_t;
typedef struct { hmac_key_id_t hmac_key_id; } nvs_sec_config_hmac_t;
typedef struct { uint8_t bytes[64]; } nvs_sec_cfg_t;
const esp_partition_t *esp_partition_find_first(int,int,const char *);
int esp_efuse_get_key_purpose(esp_efuse_block_t);
bool esp_efuse_read_field_bit(const esp_efuse_desc_t **);
esp_err_t nvs_sec_provider_register_hmac(const nvs_sec_config_hmac_t *,nvs_sec_scheme_t **);
esp_err_t nvs_flash_read_security_cfg_v2(nvs_sec_scheme_t *,nvs_sec_cfg_t *);
esp_err_t nvs_flash_secure_init_partition(const char *,nvs_sec_cfg_t *);
esp_err_t nvs_open_from_partition(const char *,const char *,int,nvs_handle_t *);
esp_err_t nvs_get_blob(nvs_handle_t,const char *,void *,size_t *);
esp_err_t nvs_set_blob(nvs_handle_t,const char *,const void *,size_t);
esp_err_t nvs_commit(nvs_handle_t);
void nvs_close(nvs_handle_t);
#endif
