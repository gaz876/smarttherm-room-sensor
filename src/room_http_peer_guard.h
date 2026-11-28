#ifndef ROOM_HTTP_PEER_GUARD_H
#define ROOM_HTTP_PEER_GUARD_H
#include <stdbool.h>
#include <stdint.h>

/* Offline policy boundary. All evidence must come from the local SDK/AP
 * observation provider, not HTTP fields. An association token must identify
 * one continuous link lifetime, not merely an address or a station index.
 * Transport membership is not cryptographic user authentication. */
typedef struct {
    bool current_ap, single_peer, endpoint_matches;
    uint64_t association;
    uint64_t peer;
} room_http_peer_observation;
typedef struct {
    uint64_t association,peer;
    bool bound,failed;
} room_http_peer_guard;
/* HTTP task only, one fresh instance per confirmed server lifetime. */
static inline bool room_http_peer_guard_check(room_http_peer_guard *g,
    bool observed,const room_http_peer_observation *o){
    if(!g||g->failed)return false;
    bool valid=observed&&o&&o->current_ap&&o->single_peer&&o->endpoint_matches&&o->association&&o->peer;
    if(!valid){if(g->bound)g->failed=true;return false;}
    if(!g->bound){g->association=o->association;g->peer=o->peer;g->bound=true;return true;}
    if(g->association!=o->association||g->peer!=o->peer){g->failed=true;return false;}
    return true;
}
#endif
