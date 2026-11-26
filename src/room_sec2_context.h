#ifndef ROOM_SEC2_CONTEXT_H
#define ROOM_SEC2_CONTEXT_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <protocomm.h>
#include <protocomm_security2.h>
#include "room_wifi_enrollment.h"

/* Experimental SDK context only. No HTTP, Wi-Fi, or device credentials.
 * Single owner, initialized to zero, never moved while live. Identity is
 * copied into owned storage; handler context is borrowed until destroy. Never
 * attach a transport until its independent security review is complete. */
enum { ROOM_SEC2_SALT_MAX=64, ROOM_SEC2_VERIFIER_SIZE=384 };
typedef struct {
    char salt[ROOM_SEC2_SALT_MAX], verifier[ROOM_SEC2_VERIFIER_SIZE];
    protocomm_security2_params_t identity;
    protocomm_t *pc;
    uint32_t session;
    bool used, failed;
} room_sec2_context;

static inline void room_sec2_clear_identity(room_sec2_context *c) {
    enroll_wipe(c->salt,sizeof(c->salt));
    enroll_wipe(c->verifier,sizeof(c->verifier));
    c->identity=(protocomm_security2_params_t){0};
}
static inline bool room_sec2_create(room_sec2_context *c,
    const protocomm_security2_params_t *identity, protocomm_req_handler_t handler, void *ctx) {
    if (!c || c->pc || c->failed || !identity || !handler || !identity->salt ||
        identity->salt_len<8 || identity->salt_len>ROOM_SEC2_SALT_MAX || !identity->verifier ||
        identity->verifier_len!=ROOM_SEC2_VERIFIER_SIZE) return false;
    protocomm_t *pc=protocomm_new();
    if (!pc) return false;
    memcpy(c->salt,identity->salt,identity->salt_len);
    memcpy(c->verifier,identity->verifier,identity->verifier_len);
    c->identity=(protocomm_security2_params_t){.salt=c->salt,.salt_len=identity->salt_len,
        .verifier=c->verifier,.verifier_len=identity->verifier_len};
    if (protocomm_set_security(pc,"room-session",&protocomm_security2,&c->identity)!=ESP_OK ||
        protocomm_add_endpoint(pc,"room-command",handler,ctx)!=ESP_OK) {
        protocomm_delete(pc); room_sec2_clear_identity(c); return false;
    }
    c->pc=pc; c->used=false; c->session=0; return true;
}
/* A context has exactly one SDK session. Never reopen it after a close.
 * The SDK session ID comes from the owner, not from a request. It does NOT
 * prove authentication. The real transport must bind a separate generation. */
static inline bool room_sec2_open(room_sec2_context *c,uint32_t session) {
    if (!c || !c->pc || c->failed || c->used || !session || session==UINT32_MAX) return false;
    c->used=true;
    if (protocomm_open_session(c->pc,session)!=ESP_OK) { c->failed=true; return false; }
    c->session=session; return true;
}
/* Owner calls only after request processing and all transport callbacks have
 * actually stopped. Failure quarantines the context instead of guessing at
 * SDK state. No automatic retry or deletion of an ambiguous live context. */
static inline bool room_sec2_destroy(room_sec2_context *c,bool drained) {
    if (!c || !drained || c->failed) return false;
    if (!c->pc) return true;
    if (c->session && protocomm_close_session(c->pc,c->session)!=ESP_OK) {
        c->failed=true; return false;
    }
    protocomm_delete(c->pc); c->pc=NULL; c->session=0; c->used=false;
    room_sec2_clear_identity(c); return true;
}
#endif
