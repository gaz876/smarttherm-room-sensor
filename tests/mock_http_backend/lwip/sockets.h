#ifndef MOCK_LWIP_SOCKETS_H
#define MOCK_LWIP_SOCKETS_H
#include <sys/socket.h>
#include <netinet/in.h>
#define LWIP_IPV6 1
#define getsockname mock_getsockname
#define getpeername mock_getpeername
int mock_getsockname(int,struct sockaddr *,socklen_t *);
int mock_getpeername(int,struct sockaddr *,socklen_t *);
#endif
