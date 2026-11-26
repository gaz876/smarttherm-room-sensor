#include "mock_sdk.h"
#include "room_runtime.h"
#include <assert.h>
#include <math.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
static jmp_buf done;
static uint64_t clock_ms, submitted_ms;
static unsigned stage, writes, commits, reads, bus_creates, bus_deletes, failed_reads;
static bool locked, bus, device, fail_commit, corrupt_readback, fail_sensor, init_fail;
static uint8_t record[ROOM_SETTINGS_BYTES], pending_record[ROOM_SETTINGS_BYTES];
static uint32_t ticket, previous_ticket;
static int bus_marker, dev_marker;
void app_main(void);
int64_t esp_timer_get_time(void) { assert(!locked); return (int64_t)clock_ms*1000; }
void portENTER_CRITICAL(portMUX_TYPE *p) { assert(!locked && !*p); locked=true; *p=1; }
void portEXIT_CRITICAL(portMUX_TYPE *p) { assert(locked && *p); *p=0; locked=false; }
void mock_log(const char *tag,const char *fmt,...) { (void)tag;(void)fmt;assert(!locked); }
esp_err_t nvs_flash_init(void) { assert(!locked); return init_fail?99:ESP_OK; }
esp_err_t nvs_open(const char *ns,nvs_open_mode_t mode,nvs_handle_t *h) {
    assert(!locked && !init_fail && !strcmp(ns,"room_measure")); *h=mode==NVS_READONLY?1:2; return ESP_OK;
}
esp_err_t nvs_get_blob(nvs_handle_t h,const char *key,void *p,size_t *len) {
    assert(!locked && h==1 && !strcmp(key,"settings") && *len==sizeof(record));
    ++reads; memcpy(p,record,sizeof(record)); *len=sizeof(record); return ESP_OK;
}
esp_err_t nvs_set_blob(nvs_handle_t h,const char *key,const void *p,size_t n) {
    assert(!locked && h==2 && !strcmp(key,"settings") && n==sizeof(record));
    ++writes; memcpy(pending_record,p,n); return ESP_OK;
}
esp_err_t nvs_commit(nvs_handle_t h) {
    assert(!locked && h==2); ++commits;
    if(fail_commit) { fail_commit=false; return 99; }
    memcpy(record,pending_record,sizeof(record));
    if(corrupt_readback) { corrupt_readback=false; record[4]^=1; }
    return ESP_OK;
}
void nvs_close(nvs_handle_t h) { assert(!locked && (h==1 || h==2)); }
esp_err_t i2c_new_master_bus(const i2c_master_bus_config_t *c,i2c_master_bus_handle_t *h) {
    assert(!locked && !bus && !device && c->sda_io_num==4 && c->scl_io_num==5);
    bus=true; ++bus_creates; *h=&bus_marker; return ESP_OK;
}
esp_err_t i2c_master_bus_add_device(i2c_master_bus_handle_t h,const i2c_device_config_t *c,i2c_master_dev_handle_t *d) {
    assert(!locked && bus && !device && h==&bus_marker && c->device_address==0x44);
    device=true; *d=&dev_marker; return ESP_OK;
}
esp_err_t i2c_master_bus_reset(i2c_master_bus_handle_t h) { assert(!locked && bus && device && h==&bus_marker);return ESP_OK; }
esp_err_t i2c_master_bus_rm_device(i2c_master_dev_handle_t h) { assert(!locked && bus && device && h==&dev_marker);device=false;return ESP_OK; }
esp_err_t i2c_del_master_bus(i2c_master_bus_handle_t h) { assert(!locked && bus && !device && h==&bus_marker);bus=false;++bus_deletes;return ESP_OK; }
esp_err_t i2c_master_transmit(i2c_master_dev_handle_t h,const uint8_t *p,size_t n,int timeout) {
    assert(!locked && bus && device && h==&dev_marker && n==1 && timeout==50 && (*p==0xfd || *p==0x94)); return ESP_OK;
}
esp_err_t i2c_master_receive(i2c_master_dev_handle_t h,uint8_t *p,size_t n,int timeout) {
    assert(!locked && bus && device && h==&dev_marker && n==6 && timeout==50);
    if(fail_sensor) { ++failed_reads; return 99; }
    p[0]=0x66;p[1]=0x66;p[2]=sht40_crc(p[0],p[1]);p[3]=0x80;p[4]=0;p[5]=sht40_crc(p[3],p[4]);return ESP_OK;
}
static void expect_settings(uint32_t interval,int32_t correction,room_settings_result status) {
    room_settings active={0};room_settings_result actual;
    assert(room_runtime_settings(&active,&actual));
    assert(active.interval_ms==interval && active.correction_mdeg==correction && actual==status);
}
static void request(room_settings s,bool reset) {
    previous_ticket=ticket;
    assert(room_runtime_request_settings(s,reset,&ticket));
    assert(ticket!=0 && ticket!=previous_ticket);
    assert(room_runtime_settings_result(ticket,NULL)==ROOM_COMMAND_PENDING);
    assert(room_runtime_settings_result(previous_ticket,NULL)==ROOM_COMMAND_UNKNOWN);
    uint32_t busy=0;
    assert(!room_runtime_request_settings(s,reset,&busy) && busy==0);
    submitted_ms=clock_ms;
}
void vTaskDelay(TickType_t ticks) {
    assert(!locked); clock_ms+=ticks*10u; if(clock_ms>=100000) { fprintf(stderr,"timeout stage=%u bus_creates=%u bus_deletes=%u writes=%u\n",stage,bus_creates,bus_deletes,writes);assert(0); }
    sht40_sample sample;room_settings_result config,result;
    room_status measurement=room_runtime_snapshot(&sample,&config);
    room_command_state state=room_runtime_settings_result(ticket,&result);
    if(init_fail) {
        if(stage==0) { expect_settings(10000,0,ROOM_SETTINGS_IO_ERROR);request((room_settings){1000,500},false);stage=1; }
        else if(state==ROOM_COMMAND_DONE) { assert(result==ROOM_SETTINGS_IO_ERROR && writes==0 && commits==0 && reads==0);expect_settings(10000,0,ROOM_SETTINGS_IO_ERROR);longjmp(done,1); }
        return;
    }
    switch(stage) {
    case 0:
        if(measurement!=ROOM_OK)break;
        expect_settings(15000,-1250,ROOM_SETTINGS_OK);assert(fabs(sample.temperature-23.75)<0.00001);
        assert(!room_runtime_request_settings((room_settings){999,0},false,&ticket));
        request((room_settings){1000,2000},false);stage++;break;
    case 1:
        if(state!=ROOM_COMMAND_DONE)break;
        assert(result==ROOM_SETTINGS_OK && writes==1 && commits==1 && reads==3);
        expect_settings(1000,2000,ROOM_SETTINGS_OK);
        if(measurement!=ROOM_OK) { assert(!sample.valid);break; }
        assert(fabs(sample.temperature-27.0)<0.00001);request((room_settings){1000,2000},false);stage++;break;
    case 2:
        if(state!=ROOM_COMMAND_DONE)break;
        assert(result==ROOM_SETTINGS_UNCHANGED && writes==1 && commits==1);
        fail_commit=true;request((room_settings){2000,-1000},false);stage++;break;
    case 3:
        if(state!=ROOM_COMMAND_DONE)break;
        assert(result==ROOM_SETTINGS_IO_ERROR && writes==2 && commits==2);expect_settings(1000,2000,ROOM_SETTINGS_IO_ERROR);
        corrupt_readback=true;request((room_settings){3000,1000},false);stage++;break;
    case 4:
        if(state!=ROOM_COMMAND_DONE)break;
        assert(result==ROOM_SETTINGS_IO_ERROR && writes==3 && commits==3);expect_settings(1000,2000,ROOM_SETTINGS_IO_ERROR);
        record[2]=2;request((room_settings){1000,0},false);stage++;break;
    case 5:
        if(state!=ROOM_COMMAND_DONE)break;
        assert(result==ROOM_SETTINGS_UNSUPPORTED && writes==3);expect_settings(1000,2000,ROOM_SETTINGS_UNSUPPORTED);
        request((room_settings){0,0},true);stage++;break;
    case 6:
        if(state!=ROOM_COMMAND_DONE)break;
        assert(result==ROOM_SETTINGS_OK && writes==4 && commits==4);expect_settings(10000,0,ROOM_SETTINGS_OK);
        if(measurement!=ROOM_OK)break;
        assert(fabs(sample.temperature-25.0)<0.00001);fail_sensor=true;request((room_settings){1000,0},false);stage++;break;
    case 7:
        if(state!=ROOM_COMMAND_DONE)break;
        assert(result==ROOM_SETTINGS_OK);expect_settings(1000,0,ROOM_SETTINGS_OK);
        /* After three failed samples, submit while backoff is pending: bus still owned. */
        if(measurement==ROOM_IO_ERROR && failed_reads>=3 && bus_deletes==0) {
            request((room_settings){2000,-2000},false);stage++;
        }
        break;
    case 8:
        if(state!=ROOM_COMMAND_DONE)break;
        assert(result==ROOM_SETTINGS_OK && clock_ms-submitted_ms<500 && bus_deletes==0);
        expect_settings(2000,-2000,ROOM_SETTINGS_OK);fail_sensor=false;stage++;break;
    case 9:
        if(bus_creates<2 || measurement!=ROOM_OK)break;
        assert(fabs(sample.temperature-23.0)<0.00001);expect_settings(2000,-2000,ROOM_SETTINGS_OK);
        assert(writes==6 && commits==6 && bus_deletes==1);longjmp(done,1);
    default: assert(0);
    }
}
int main(void) {
    room_settings s;uint32_t id=0;
    assert(!room_runtime_settings(&s,NULL));assert(!room_runtime_request_settings((room_settings){1000,0},false,&id));
    assert(room_settings_encode((room_settings){15000,-1250},record));
    if(!setjmp(done)) { app_main();assert(0); }
    assert(stage==9);
    stage=0;clock_ms=0;ticket=0;writes=commits=reads=0;bus=device=locked=false;init_fail=true;
    if(!setjmp(done)) { app_main();assert(0); }
    assert(stage==1);
    puts("PASS: runtime NVS requests, busy/tickets, apply after readback, no-op, failed commit, mismatched readback, unknown format/reset, recovery responsiveness, failed init");
}
