/* Test-only access to the actual private task. Never compiled into firmware.
 * No public packet/listener entry point is added to the production link. */
#include "../main/room_wifi_apsta_link.c"
bool fixture_packet(bool hs,const uint8_t *in,size_t len,const uint8_t **out,size_t *n) {
    return room_apsta_task_packet(&a.task,hs,in,len,out,n);
}
bool fixture_sent(bool sent) { return room_apsta_task_sent(&a.task,sent); }

bool fixture_identity_owned(void) {
    const room_sec2_context *c=&a.task.session.transport.sec;
    return c->pc && c->identity.salt==c->salt && c->identity.verifier==c->verifier &&
        c->identity.salt_len==16 && c->identity.verifier_len==384 &&
        c->salt[0]=='s' && c->verifier[0]=='v';
}
bool fixture_identity_cleared(void) {
    const room_sec2_context *c=&a.task.session.transport.sec;
    if(c->pc||c->identity.salt||c->identity.verifier)return false;
    for(size_t i=0;i<sizeof(c->salt);i++)if(c->salt[i])return false;
    for(size_t i=0;i<sizeof(c->verifier);i++)if(c->verifier[i])return false;
    return true;
}
