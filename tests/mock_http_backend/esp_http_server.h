#ifndef MOCK_HTTP_H
#define MOCK_HTTP_H
#include <stddef.h>
#include <stdbool.h>
typedef void *httpd_handle_t;
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1
#define HTTP_POST 1
struct ifreq;
typedef struct { void *user_ctx; size_t content_len; httpd_handle_t handle; } httpd_req_t;
typedef struct { const char *uri; int method; esp_err_t (*handler)(httpd_req_t *); void *user_ctx; } httpd_uri_t;
typedef struct { struct ifreq *if_name; unsigned max_open_sockets,max_uri_handlers,recv_wait_timeout,send_wait_timeout; bool lru_purge_enable; } httpd_config_t;
#define HTTPD_DEFAULT_CONFIG() ((httpd_config_t){0})
esp_err_t httpd_start(httpd_handle_t *,const httpd_config_t *);
esp_err_t httpd_stop(httpd_handle_t);
esp_err_t httpd_register_uri_handler(httpd_handle_t,const httpd_uri_t *);
int httpd_req_recv(httpd_req_t *,char *,size_t);
esp_err_t httpd_resp_set_type(httpd_req_t *,const char *);
esp_err_t httpd_resp_set_hdr(httpd_req_t *,const char *,const char *);
esp_err_t httpd_resp_send(httpd_req_t *,const char *,size_t);
int httpd_req_to_sockfd(httpd_req_t *);
#endif
