#include "room_settings_nvs.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int open_err,get_err,set_err,commit_err;
static unsigned opens,closes,gets,sets,commits;
static bool opened;
static nvs_open_mode_t mode;
static uint8_t blob[ROOM_SETTINGS_BYTES];
static size_t blob_len;
static void reset_fake(void) {
    assert(!opened);
    open_err=get_err=set_err=commit_err=0;
    opens=closes=gets=sets=commits=0;
    blob_len=sizeof(blob);
}
esp_err_t nvs_open(const char *ns,nvs_open_mode_t m,nvs_handle_t *h) {
    assert(!opened && strcmp(ns,"room_measure")==0); ++opens;
    if(open_err) return open_err;
    opened=true; mode=m; *h=42; return ESP_OK;
}
esp_err_t nvs_get_blob(nvs_handle_t h,const char *key,void *p,size_t *len) {
    assert(opened && mode==NVS_READONLY && h==42 && strcmp(key,"settings")==0); ++gets;
    if(get_err) return get_err;
    if(*len<blob_len) return ESP_ERR_NVS_INVALID_LENGTH;
    memcpy(p,blob,blob_len); *len=blob_len; return ESP_OK;
}
esp_err_t nvs_set_blob(nvs_handle_t h,const char *key,const void *p,size_t len) {
    assert(opened && mode==NVS_READWRITE && h==42 && strcmp(key,"settings")==0); ++sets;
    if(set_err) return set_err;
    assert(len==sizeof(blob)); memcpy(blob,p,len); blob_len=len; return ESP_OK;
}
esp_err_t nvs_commit(nvs_handle_t h) {
    assert(opened && mode==NVS_READWRITE && h==42); ++commits; return commit_err;
}
void nvs_close(nvs_handle_t h) { assert(opened && h==42); opened=false; ++closes; }
int main(void) {
    const room_settings defaults={10000,0}, custom={20000,1234}; room_settings out;
    const room_settings_store s=room_nvs_store();
    reset_fake(); open_err=ESP_ERR_NVS_NOT_FOUND;
    assert(room_settings_load(&s,defaults,&out)==ROOM_SETTINGS_MISSING && closes==0 && gets==0);
    reset_fake(); get_err=ESP_ERR_NVS_NOT_FOUND;
    assert(room_settings_load(&s,defaults,&out)==ROOM_SETTINGS_MISSING && closes==1 && sets==0);
    reset_fake(); get_err=ESP_ERR_NVS_TYPE_MISMATCH;
    assert(room_settings_load(&s,defaults,&out)==ROOM_SETTINGS_UNSUPPORTED && closes==1);
    reset_fake(); blob_len=sizeof(blob)+1;
    assert(room_settings_load(&s,defaults,&out)==ROOM_SETTINGS_UNSUPPORTED && closes==1);
    reset_fake(); get_err=99;
    assert(room_settings_save(&s,custom,false)==ROOM_SETTINGS_IO_ERROR && opens==1 && closes==1 && sets==0);
    reset_fake(); get_err=ESP_ERR_NVS_NOT_FOUND; set_err=99;
    assert(room_settings_save(&s,custom,false)==ROOM_SETTINGS_IO_ERROR && opens==2 && closes==2 && commits==0);
    reset_fake(); get_err=ESP_ERR_NVS_NOT_FOUND; commit_err=99;
    assert(room_settings_save(&s,custom,false)==ROOM_SETTINGS_IO_ERROR && opens==2 && closes==2 && commits==1);
    reset_fake(); get_err=ESP_ERR_NVS_NOT_FOUND;
    assert(room_settings_save(&s,custom,false)==ROOM_SETTINGS_OK && opens==2 && closes==2 && sets==1 && commits==1);
    reset_fake();
    assert(room_settings_load(&s,defaults,&out)==ROOM_SETTINGS_OK && out.interval_ms==custom.interval_ms && out.correction_mdeg==custom.correction_mdeg);
    assert(room_settings_save(&s,custom,false)==ROOM_SETTINGS_UNCHANGED && sets==0 && commits==0 && closes==2);
    assert(room_settings_save(&s,defaults,true)==ROOM_SETTINGS_OK && sets==1 && commits==1 && closes==4);
    puts("PASS: NVS adapter mock namespace/key, read-only, missing/type/size errors, set/commit failure, handles, save/reset/no-op");
}
