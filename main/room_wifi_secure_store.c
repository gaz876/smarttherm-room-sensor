#include "room_wifi_secure_store.h"
#include "esp_efuse.h"
#include "esp_efuse_table.h"
#include "esp_partition.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "nvs_sec_provider.h"

#define PART "room_wifi"
#define NS "network"
#define KEY "profile"
#define RECORD_SIZE 98u
static bool attempted, ready, quarantined;
/* Provider registration copies a pointer into the SDK default scheme; retain
 * the handle for the process lifetime. Do not deregister it. */
static nvs_sec_scheme_t *scheme;

bool room_secure_init(unsigned key_id) {
    if (attempted) return false;
    attempted=true;
    if (key_id>=6u) return false;
    const esp_efuse_desc_t **const rd[]={ESP_EFUSE_RD_DIS_KEY0,ESP_EFUSE_RD_DIS_KEY1,ESP_EFUSE_RD_DIS_KEY2,ESP_EFUSE_RD_DIS_KEY3,ESP_EFUSE_RD_DIS_KEY4,ESP_EFUSE_RD_DIS_KEY5};
    const esp_efuse_desc_t **const wr[]={ESP_EFUSE_WR_DIS_KEY0,ESP_EFUSE_WR_DIS_KEY1,ESP_EFUSE_WR_DIS_KEY2,ESP_EFUSE_WR_DIS_KEY3,ESP_EFUSE_WR_DIS_KEY4,ESP_EFUSE_WR_DIS_KEY5};
    const esp_partition_t *part=esp_partition_find_first(ESP_PARTITION_TYPE_DATA,ESP_PARTITION_SUBTYPE_DATA_NVS,PART);
    if (!part || part->encrypted || part->readonly || part->size<0x3000u) return false;
    if (esp_efuse_get_key_purpose((esp_efuse_block_t)(EFUSE_BLK_KEY0+key_id))!=ESP_EFUSE_KEY_PURPOSE_HMAC_UP ||
        !esp_efuse_read_field_bit(rd[key_id]) || !esp_efuse_read_field_bit(wr[key_id])) return false;
    nvs_sec_config_hmac_t h={.hmac_key_id=(hmac_key_id_t)key_id};
    nvs_sec_cfg_t cfg={0};
    bool ok=nvs_sec_provider_register_hmac(&h,&scheme)==ESP_OK && scheme &&
        nvs_flash_read_security_cfg_v2(scheme,&cfg)==ESP_OK;
    /* No generation API, default partition init, erase or NULL-key fallback. */
    if (ok) ok=nvs_flash_secure_init_partition(PART,&cfg)==ESP_OK;
    enroll_wipe(&cfg,sizeof(cfg));
    ready=ok;
    return ok;
}
static bool usable(void *ctx) { (void)ctx; return ready&&!quarantined; }
static bool canonical(const room_enroll_profile *p) {
    if (!enroll_profile_valid(p->ssid,p->ssid_len,p->pass,p->pass_len)) return false;
    for (size_t i=p->ssid_len;i<sizeof(p->ssid);i++) if(p->ssid[i]) return false;
    for (size_t i=p->pass_len;i<sizeof(p->pass);i++) if(p->pass[i]) return false;
    return true;
}
room_secure_status room_secure_load(room_enroll_profile *out) {
    if (!out) return ROOM_SEC_ERROR;
    enroll_wipe(out,sizeof(*out));
    if (!usable(NULL)) return ROOM_SEC_UNAVAILABLE;
    nvs_handle_t h;
    esp_err_t err=nvs_open_from_partition(PART,NS,NVS_READONLY,&h);
    if (err==ESP_ERR_NVS_NOT_FOUND) return ROOM_SEC_MISSING;
    if (err!=ESP_OK) { quarantined=true; return ROOM_SEC_ERROR; }
    uint8_t bytes[RECORD_SIZE]={0}; size_t len=sizeof(bytes);
    err=nvs_get_blob(h,KEY,bytes,&len); nvs_close(h);
    room_secure_status status=ROOM_SEC_ERROR;
    if (err==ESP_ERR_NVS_NOT_FOUND) status=ROOM_SEC_MISSING;
    else if (err==ESP_OK && len==sizeof(bytes) && bytes[0]==1u) {
        out->ssid_len=bytes[1]; out->pass_len=bytes[2];
        memcpy(out->ssid,bytes+3,32); memcpy(out->pass,bytes+35,63);
        if (canonical(out)) status=ROOM_SEC_VALID;
    }
    enroll_wipe(bytes,sizeof(bytes));
    if (status!=ROOM_SEC_VALID) enroll_wipe(out,sizeof(*out));
    if (status==ROOM_SEC_ERROR) quarantined=true;
    return status;
}
static bool read_profile(void *ctx,room_enroll_profile *p) { (void)ctx; return room_secure_load(p)==ROOM_SEC_VALID; }
static bool write_profile(void *ctx,const room_enroll_profile *p) {
    (void)ctx;
    if (!usable(NULL)||!p||!canonical(p)) return false;
    uint8_t bytes[RECORD_SIZE]={1u,p->ssid_len,p->pass_len};
    memcpy(bytes+3,p->ssid,32); memcpy(bytes+35,p->pass,63);
    nvs_handle_t h;
    bool ok=nvs_open_from_partition(PART,NS,NVS_READWRITE,&h)==ESP_OK;
    if (ok) { ok=nvs_set_blob(h,KEY,bytes,sizeof(bytes))==ESP_OK && nvs_commit(h)==ESP_OK; nvs_close(h); }
    enroll_wipe(bytes,sizeof(bytes));
    if (!ok) quarantined=true;
    return ok;
}
room_enroll_store room_secure_store(void) { return (room_enroll_store){NULL,usable,write_profile,read_profile}; }

/* Same explicitly initialized encrypted partition; separate namespace.
 * Identity corruption disables setup, but must not erase or overwrite the
 * router profile. Storage readiness remains independently enforced. */
room_secure_status room_secure_identity_load(room_setup_identity *out) {
    if(!out)return ROOM_SEC_ERROR;
    enroll_wipe(out,sizeof(*out));
    if(!usable(NULL))return ROOM_SEC_UNAVAILABLE;
    nvs_handle_t h;
    esp_err_t err=nvs_open_from_partition(PART,"setup_id",NVS_READONLY,&h);
    if(err==ESP_ERR_NVS_NOT_FOUND)return ROOM_SEC_MISSING;
    if(err!=ESP_OK)return ROOM_SEC_ERROR;
    uint8_t bytes[ROOM_ID_RECORD_SIZE]={0};size_t n=sizeof(bytes);
    err=nvs_get_blob(h,"identity",bytes,&n);nvs_close(h);
    room_secure_status result=ROOM_SEC_ERROR;
    if(err==ESP_ERR_NVS_NOT_FOUND)result=ROOM_SEC_MISSING;
    else if(err==ESP_OK&&room_id_decode(out,bytes,n))result=ROOM_SEC_VALID;
    enroll_wipe(bytes,sizeof(bytes));
    return result;
}
