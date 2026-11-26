#include "mock_sdk.h"
#include "room_runtime.h"
#include <assert.h>
#include <math.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
static jmp_buf done;
static uint64_t clock_ms, starts[6];
static unsigned attempts, reads, creates, destroys, nvs_inits, nvs_reads, nvs_writes;
static unsigned ok_before, ok_after, invalid_seen, stale_seen;
static bool bus, device, locked;
static int nvs_init_error;
static uint8_t record[ROOM_SETTINGS_BYTES];
static room_settings_result want_status;
static uint32_t want_interval;
static double want_correction;
static int bus_marker, dev_marker;
void app_main(void);
int64_t esp_timer_get_time(void) { assert(!locked); return (int64_t)clock_ms*1000; }
void portENTER_CRITICAL(portMUX_TYPE *p) { assert(!locked && !*p); locked=true; *p=1; }
void portEXIT_CRITICAL(portMUX_TYPE *p) { assert(locked && *p); *p=0; locked=false; }
void mock_log(const char *tag,const char *fmt,...) { (void)tag;(void)fmt;assert(!locked); }
esp_err_t nvs_flash_init(void) { assert(!locked); ++nvs_inits; return nvs_init_error; }
esp_err_t nvs_open(const char *ns,nvs_open_mode_t m,nvs_handle_t *h) {
    assert(!locked && m==NVS_READONLY && !strcmp(ns,"room_measure")); *h=1; return ESP_OK;
}
esp_err_t nvs_get_blob(nvs_handle_t h,const char *key,void *p,size_t *len) {
    assert(!locked && h==1 && !strcmp(key,"settings") && *len==sizeof(record));
    ++nvs_reads; memcpy(p,record,sizeof(record)); *len=sizeof(record);return ESP_OK;
}
esp_err_t nvs_set_blob(nvs_handle_t h,const char *key,const void *p,size_t n) {
    (void)h;(void)key;(void)p;(void)n;++nvs_writes;return 99;
}
esp_err_t nvs_commit(nvs_handle_t h) { (void)h;++nvs_writes;return 99; }
void nvs_close(nvs_handle_t h) { assert(!locked && h==1); }
esp_err_t i2c_new_master_bus(const i2c_master_bus_config_t *c,i2c_master_bus_handle_t *h) {
    assert(!locked && !bus && !device && c->sda_io_num==4 && c->scl_io_num==5);
    bus=true; ++creates;*h=&bus_marker; return ESP_OK;
}
esp_err_t i2c_master_bus_add_device(i2c_master_bus_handle_t h,const i2c_device_config_t *c,i2c_master_dev_handle_t *d) {
    assert(!locked && bus && !device && h==&bus_marker && c->device_address==0x44);
    device=true;*d=&dev_marker;return ESP_OK;
}
esp_err_t i2c_master_bus_reset(i2c_master_bus_handle_t h) { assert(!locked && bus && device && h==&bus_marker);return ESP_OK; }
esp_err_t i2c_master_bus_rm_device(i2c_master_dev_handle_t h) { assert(!locked && bus && device && h==&dev_marker);device=false;return ESP_OK; }
esp_err_t i2c_del_master_bus(i2c_master_bus_handle_t h) { assert(!locked && bus && !device && h==&bus_marker);bus=false;++destroys;return ESP_OK; }
esp_err_t i2c_master_transmit(i2c_master_dev_handle_t h,const uint8_t *p,size_t n,int timeout) {
    assert(!locked && bus && device && h==&dev_marker && n==1 && timeout==50);
    if(*p==0xfd) { assert(attempts<6);starts[attempts++]=clock_ms; }
    else assert(*p==0x94);
    return ESP_OK;
}
esp_err_t i2c_master_receive(i2c_master_dev_handle_t h,uint8_t *p,size_t n,int timeout) {
    assert(!locked && bus && device && h==&dev_marker && n==6 && timeout==50);
    ++reads; if(reads>=2 && reads<=4)return 99;
    p[0]=0x66;p[1]=0x66;p[2]=sht40_crc(p[0],p[1]);p[3]=0x80;p[4]=0;p[5]=sht40_crc(p[3],p[4]);return ESP_OK;
}
void vTaskDelay(TickType_t ticks) {
    assert(!locked);clock_ms+=ticks*10u;
    sht40_sample s;room_settings_result config;
    room_status status=room_runtime_snapshot(&s,&config);
    assert(config==want_status);
    if(status==ROOM_OK) {
        assert(s.valid && fabs(s.temperature-(25.0+want_correction))<0.00001);
        assert(fabs(s.humidity-(-6.0+125.0*32768.0/65535.0))<0.00001);
        if(reads==1)++ok_before;
        if(reads==5)++ok_after;
    } else {
        assert(!s.valid && s.temperature==0 && s.humidity==0);
        if(status==ROOM_IO_ERROR)++invalid_seen;
        if(status==ROOM_STALE)++stale_seen;
    }
    if(ok_after)longjmp(done,1);
    assert(clock_ms<100000);
}
static void run_case(int init_error,room_settings_result status,uint32_t interval,double correction) {
    clock_ms=0;attempts=reads=creates=destroys=nvs_inits=nvs_reads=nvs_writes=0;
    ok_before=ok_after=invalid_seen=stale_seen=0;bus=device=locked=false;
    nvs_init_error=init_error;want_status=status;want_interval=interval;want_correction=correction;
    if(!setjmp(done)) { app_main();assert(0); }
    assert(ok_before && ok_after && invalid_seen && stale_seen);
    assert(attempts==5 && reads==5 && creates==2 && destroys==1);
    assert(nvs_inits==1 && nvs_reads==(init_error?0u:1u) && nvs_writes==0);
    for(unsigned i=1;i<4;i++)assert(starts[i]-starts[i-1]>=want_interval-20 && starts[i]-starts[i-1]<want_interval+100);
}
int main(void) {
    sht40_sample s; room_settings_result status;
    assert(room_runtime_snapshot(&s,&status)==ROOM_NOT_READY && !s.valid);
    assert(room_settings_encode((room_settings){15000,-1250},record));
    run_case(0,ROOM_SETTINGS_OK,15000,-1.25);
    record[2]=2;
    run_case(0,ROOM_SETTINGS_UNSUPPORTED,10000,0);
    run_case(99,ROOM_SETTINGS_IO_ERROR,10000,0);
    puts("PASS: real app source with mock SDK: persisted cadence/correction across I2C recovery, freshness/status, defaults on NVS faults, no automatic writes, lock boundaries");
}
