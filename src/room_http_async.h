#ifndef ROOM_HTTP_ASYNC_H
#define ROOM_HTTP_ASYNC_H
#include "room_http_lifecycle.h"

/* Inactive SDK boundary. A single-use context, exclusively owned by one
 * task at a time, with synchronized handoff. Never copy it. The URI callback
 * holds its own lease until it stops using the original SDK request. The
 * async lease protects the copy and any borrowed application buffers until
 * all accesses finish, including completion. Never retry ambiguous SDK
 * failures. The closed gate and retained lease prevent unsafe SDK stop.
 * One begin per original URI invocation; caller owns that invariant.
 * No routing, network activation, queue or automatic restart here. */
typedef enum { ROOM_ASYNC_EMPTY, ROOM_ASYNC_ACTIVE, ROOM_ASYNC_DONE,
    ROOM_ASYNC_UNCERTAIN } room_http_async_state;
typedef struct {
    room_lease lease;
    httpd_req_t *request;
    room_http_async_state state;
} room_http_async;

static inline bool room_http_async_uncertain(room_http_async *a) {
    room_admission *g=a->lease.gate;
    a->request=NULL;
    a->state=ROOM_ASYNC_UNCERTAIN;
    /* Retain the lease permanently; do not mutate owner-only worker fields. */
    if(g)(void)room_admission_close(g,g->window);
    return false;
}
static inline bool room_http_async_begin(room_http_async *a,room_http_lifecycle *s,
    uint64_t window,const room_lease *callback,httpd_req_t *request) {
    if(!a||a->state!=ROOM_ASYNC_EMPTY||a->lease.gate||a->request||!s||
       !callback||callback->gate!=&s->gate||!request)return false;
    if(!room_http_enter(s,window,&a->lease))return false;
    /* The acquired lease prevents handle transfer during this comparison. */
    if(request->handle!=s->handle){
        (void)room_admission_leave(&a->lease);return false;
    }
    httpd_req_t *copy=NULL;
    esp_err_t rc=httpd_req_async_handler_begin(request,&copy);
    /* Pinned ESP-IDF 6.1.1: allocation failure cleans all partial copies
     * before publishing a request or marking its socket async. */
    if(rc==ESP_ERR_NO_MEM&&!copy){
        a->state=ROOM_ASYNC_DONE;
        (void)room_admission_leave(&a->lease);return false;
    }
    if(rc!=ESP_OK||!copy||copy==request)return room_http_async_uncertain(a);
    a->request=copy;a->state=ROOM_ASYNC_ACTIVE;return true;
}
/* Call on normal and application-error paths, only after last use of the
 * copy and every borrowed buffer. Consumes the request even on SDK error:
 * the API does not guarantee that the pointer remains valid on failure. */
static inline bool room_http_async_complete(room_http_async *a) {
    if(!a||a->state!=ROOM_ASYNC_ACTIVE||!a->request||!a->lease.gate)return false;
    httpd_req_t *copy=a->request;a->request=NULL;
    if(httpd_req_async_handler_complete(copy)!=ESP_OK)return room_http_async_uncertain(a);
    a->state=ROOM_ASYNC_DONE;
    return room_admission_leave(&a->lease);
}
#endif
