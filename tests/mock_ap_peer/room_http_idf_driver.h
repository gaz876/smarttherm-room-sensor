#ifndef MOCK_IDF_DRIVER_H
#define MOCK_IDF_DRIVER_H
#include "room_wifi_apsta_link.h"
#include "room_http_peer_guard.h"
typedef struct {int fd;} httpd_req_t;
int httpd_req_to_sockfd(httpd_req_t *);
#endif
