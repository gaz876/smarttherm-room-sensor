#ifndef MOCK_HTTP_H
#define MOCK_HTTP_H
typedef void *httpd_handle_t;
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_ERR_NO_MEM 0x101
typedef struct { httpd_handle_t handle; } httpd_req_t;
esp_err_t httpd_req_async_handler_begin(httpd_req_t *,httpd_req_t **);
esp_err_t httpd_req_async_handler_complete(httpd_req_t *);
#endif
