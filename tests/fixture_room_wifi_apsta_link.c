/* Test-only access to the actual private task. Never compiled into firmware.
 * No public packet/listener entry point is added to the production link. */
#include "../main/room_wifi_apsta_link.c"
bool fixture_packet(bool hs,const uint8_t *in,size_t len,const uint8_t **out,size_t *n) {
    return room_apsta_task_packet(&a.task,hs,in,len,out,n);
}
bool fixture_sent(bool sent) { return room_apsta_task_sent(&a.task,sent); }
