#ifndef MOCK_PROTOCOMM_H
#define MOCK_PROTOCOMM_H
#include <stdint.h>
#include <sys/types.h>
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL (-1)
typedef struct protocomm protocomm_t;
typedef struct { int ver; } protocomm_security_t;
typedef struct { const char *salt; uint16_t salt_len; const char *verifier; uint16_t verifier_len; } protocomm_security2_params_t;
typedef esp_err_t (*protocomm_req_handler_t)(uint32_t,const uint8_t *,ssize_t,uint8_t **,ssize_t *,void *);
protocomm_t *protocomm_new(void);
void protocomm_delete(protocomm_t *);
esp_err_t protocomm_set_security(protocomm_t *,const char *,const protocomm_security_t *,const void *);
esp_err_t protocomm_add_endpoint(protocomm_t *,const char *,protocomm_req_handler_t,void *);
esp_err_t protocomm_open_session(protocomm_t *,uint32_t);
esp_err_t protocomm_close_session(protocomm_t *,uint32_t);
#endif
