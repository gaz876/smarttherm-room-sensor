#ifndef MOCK_LWIP_SOCKETS_H
#define MOCK_LWIP_SOCKETS_H
#include <sys/socket.h>
#include <netinet/in.h>
/* Replace SDK socket observations; no real sockets or system network calls. */
#define getsockname mock_getsockname
#define getpeername mock_getpeername
int mock_getsockname(int,struct sockaddr *,socklen_t *);
int mock_getpeername(int,struct sockaddr *,socklen_t *);
#endif
