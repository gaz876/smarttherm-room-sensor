#include "room_wifi_identity_provider.h"
#include "room_wifi_secure_store.h"
#include "freertos/task.h"
_Static_assert((int)ROOM_ID_SALT_MAX==(int)ROOM_SEC2_SALT_MAX,"salt size mismatch");
_Static_assert((int)ROOM_ID_VERIFIER_SIZE==(int)ROOM_SEC2_VERIFIER_SIZE,"verifier size mismatch");
static struct {
    TaskHandle_t owner;
    uint32_t serial;
    bool attempted,available;
} identity;
static bool ready(void *ctx) {
    (void)ctx;
    if(!identity.available||identity.owner!=xTaskGetCurrentTaskHandle()||
        identity.serial>=UINT32_MAX-1u)return false;
    room_enroll_store store=room_secure_store();
    return enroll_ready(&store);
}
static bool prepare(void *ctx,room_wifi_apsta_window *out) {
    if(!out)return false;
    enroll_wipe(out,sizeof(*out));
    if(!ready(ctx))return false;
    room_setup_identity record={0};
    bool ok=room_secure_identity_load(&record)==ROOM_SEC_VALID;
    if(ok){
        memcpy(out->ssid,record.ssid,record.ssid_len);
        memcpy(out->key,record.key,record.key_len);
        memcpy(out->salt,record.salt,record.salt_len);
        memcpy(out->verifier,record.verifier,sizeof(record.verifier));
        out->ssid_len=record.ssid_len;out->key_len=record.key_len;
        out->salt_len=record.salt_len;out->verifier_len=sizeof(record.verifier);
        out->sdk_id=++identity.serial;
    }else identity.available=false;
    enroll_wipe(&record,sizeof(record));
    return ok;
}
bool room_wifi_identity_provider_init(room_wifi_apsta_provider *out) {
    if(!out)return false;
    *out=(room_wifi_apsta_provider){0};
    if(identity.attempted)return false;
    identity.attempted=true;identity.owner=xTaskGetCurrentTaskHandle();
    if(!identity.owner)return false;
    room_setup_identity record={0};
    identity.available=room_secure_identity_load(&record)==ROOM_SEC_VALID;
    enroll_wipe(&record,sizeof(record));
    if(!identity.available)return false;
    *out=(room_wifi_apsta_provider){NULL,ready,prepare};
    return true;
}
