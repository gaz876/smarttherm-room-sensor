#ifndef ROOM_SOCKET_IPV4_H
#define ROOM_SOCKET_IPV4_H
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "lwip/sockets.h"

/* Compare only the exact expected IPv4 address from trusted local evidence.
 * SDK dual-stack sockets can represent IPv4 in an IPv6 sockaddr.
 * Native IPv6, malformed lengths and scoped addresses remain unsupported. */
static inline bool room_socket_ipv4_matches(const struct sockaddr_storage *s,
    socklen_t len,uint32_t expected){
    if(!s||!expected)return false;
    if(len==sizeof(struct sockaddr_in)&&s->ss_family==AF_INET){
        struct sockaddr_in v;
        memcpy(&v,s,sizeof(v));
        return v.sin_addr.s_addr==expected;
    }
#if defined(LWIP_IPV6) && LWIP_IPV6
    if(len==sizeof(struct sockaddr_in6)&&s->ss_family==AF_INET6){
        struct sockaddr_in6 v;
        memcpy(&v,s,sizeof(v));
        const uint8_t prefix[12]={0,0,0,0,0,0,0,0,0,0,255,255};
        const uint8_t *bytes=(const uint8_t *)&v.sin6_addr;
        return !v.sin6_scope_id&&!v.sin6_flowinfo&&
               memcmp(bytes,prefix,sizeof(prefix))==0&&
               memcmp(bytes+sizeof(prefix),&expected,sizeof(expected))==0;
    }
#endif
    return false;
}
#endif
