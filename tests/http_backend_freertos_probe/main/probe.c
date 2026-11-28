/* Test-only, no physical interface, sockets, credentials or SDK HTTP listener.
 * Runs the unchanged backend and stop worker on the real ESP-IDF scheduler. */
#include "room_http_backend.h"
#include "freertos/semphr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(c) do { if (!(c)) { printf("BACKEND_TARGET_FAIL line=%d\n", __LINE__); abort(); } } while (0)
static room_http_backend backend;
static room_network_setup_backend port;
static TaskHandle_t owner_task, callback_task;
static SemaphoreHandle_t offered, finished, release_callback;
static room_http_backend_window *window;
static int ap_marker, server_marker, request_marker;
static const uint8_t input_bytes[] = {1, 2, 3};
static const uint8_t output_bytes[] = {4, 5, 6, 7};
static atomic_uint stop_count;
static atomic_bool fail_stop, callback_live, hold_callback;
static bool fail_start, fail_publish;
static esp_netif_t *fake_ap(void) { return (esp_netif_t *)(void *)&ap_marker; }
static uint64_t deadline(void) {
    int64_t now=esp_timer_get_time();
    CHECK(now>=0);
    return (uint64_t)now/1000u+3000u;
}
static void take(SemaphoreHandle_t sem) {
    CHECK(xSemaphoreTake(sem,pdMS_TO_TICKS(1000))==pdTRUE);
}
static void give(SemaphoreHandle_t sem) {
    CHECK(xSemaphoreGive(sem)==pdTRUE);
}
/* Unused production owner must never be entered by this standalone probe. */
bool room_network_owner_prepare(unsigned key_id,const room_network_setup_backend *b,room_secure_status *s) {
    (void)key_id; (void)b; (void)s; CHECK(false); return false;
}
static room_http_start_result start_cb(void *ctx,const esp_netif_t *ap,httpd_handle_t *h) {
    CHECK(ctx==&ap_marker && ap==fake_ap() && h);
    *h=fail_start?NULL:&server_marker;
    return fail_start?ROOM_HTTP_START_CLEAN_FAILURE:ROOM_HTTP_START_READY;
}
static bool publish_cb(void *ctx,httpd_handle_t h,room_http_backend_window *w) {
    CHECK(ctx==&ap_marker && h==&server_marker && w);
    window=w; return !fail_publish;
}
static esp_err_t stop_cb(httpd_handle_t h) {
    CHECK(h==&server_marker && xTaskGetCurrentTaskHandle()!=owner_task);
    CHECK(!atomic_load(&callback_live));
    atomic_fetch_add(&stop_count,1u);
    return atomic_load(&fail_stop)?ESP_FAIL:ESP_OK;
}
static bool peer_cb(void *ctx,const esp_netif_t *ap,const void *req) {
    return ctx==&ap_marker && ap==fake_ap() && req==&request_marker;
}
static void callback(void *unused) {
    (void)unused;
    for (;;) {
        CHECK(ulTaskNotifyTake(pdTRUE,portMAX_DELAY)==1u);
        room_http_exchange x={0};
        uint8_t buf[8]; size_t n=0;
        atomic_store(&callback_live,true);
        CHECK(room_http_backend_offer(window,&x,&request_marker,true,input_bytes,sizeof(input_bytes)));
        give(offered);
        if (atomic_load(&hold_callback)) {
            take(release_callback);
            CHECK(!room_http_backend_finish(&x,false));
        } else {
            room_phone_io_result rc=ROOM_PHONE_IO_WAIT;
            for (unsigned i=0;i<100u && rc==ROOM_PHONE_IO_WAIT;++i) {
                rc=room_http_backend_reply(&x,buf,sizeof(buf),&n);
                if(rc==ROOM_PHONE_IO_WAIT) vTaskDelay(1);
            }
            CHECK(rc==ROOM_PHONE_IO_OK && n==sizeof(output_bytes));
            CHECK(memcmp(buf,output_bytes,n)==0);
            CHECK(room_http_backend_finish(&x,true));
        }
        atomic_store(&callback_live,false);
        give(finished);
    }
}
static void launch(bool hold) {
    atomic_store(&hold_callback,hold);
    xTaskNotifyGive(callback_task);
    take(offered);
}
static void exchange(void) {
    uint8_t buf[8]; size_t n=0; bool hs=false;
    launch(false);
    CHECK(port.io.receive(port.io.ctx,&hs,buf,sizeof(buf),&n)==ROOM_PHONE_IO_OK);
    CHECK(hs && n==sizeof(input_bytes) && memcmp(buf,input_bytes,n)==0);
    CHECK(port.io.send(port.io.ctx,output_bytes,sizeof(output_bytes),&n)==ROOM_PHONE_IO_WAIT);
    take(finished);
    CHECK(port.io.send(port.io.ctx,output_bytes,sizeof(output_bytes),&n)==ROOM_PHONE_IO_OK);
    CHECK(n==sizeof(output_bytes));
}
static void finish_close(uint64_t w,uint64_t end) {
    for (unsigned i=0;i<100u;++i) {
        if(port.close_until(port.io.ctx,w,end)) {
            CHECK(!backend.current && !backend.failed);
            CHECK(port.close_until(port.io.ctx,w,end));
            return;
        }
        CHECK(!backend.failed);
        vTaskDelay(1);
    }
    CHECK(false);
}
void app_main(void) {
    owner_task=xTaskGetCurrentTaskHandle();
    atomic_init(&stop_count,0); atomic_init(&fail_stop,false);
    atomic_init(&callback_live,false); atomic_init(&hold_callback,false);
    offered=xSemaphoreCreateBinary(); finished=xSemaphoreCreateBinary(); release_callback=xSemaphoreCreateBinary();
    CHECK(offered && finished && release_callback);
    CHECK(xTaskCreate(callback,"backend_probe",4096,NULL,3,&callback_task)==pdPASS);
    room_http_driver d={&ap_marker,start_cb,publish_cb,stop_cb,peer_cb};
    CHECK(room_http_backend_prepare(&backend,d,&port));
    fail_start=true;
    CHECK(!port.io.open(port.io.ctx,fake_ap(),1));
    finish_close(1,deadline()); CHECK(atomic_load(&stop_count)==0);
    fail_start=false; fail_publish=true;
    CHECK(!port.io.open(port.io.ctx,fake_ap(),2));
    finish_close(2,deadline()); CHECK(atomic_load(&stop_count)==1);
    fail_publish=false;
    for (uint64_t w=3;w<=34;++w) {
        CHECK(port.io.open(port.io.ctx,fake_ap(),w));
        CHECK(!port.io.open(port.io.ctx,fake_ap(),w+1));
        exchange(); exchange();
        finish_close(w,deadline());
        CHECK(atomic_load(&stop_count)==w-1);
    }
    printf("BACKEND_TARGET_RESTART_PASS windows=32 exchanges=64 stops=33\n");
    CHECK(port.io.open(port.io.ctx,fake_ap(),35));
    launch(true);
    uint64_t end=deadline();
    CHECK(!port.close_until(port.io.ctx,35,end));
    CHECK(atomic_load(&stop_count)==33 && atomic_load(&callback_live));
    give(release_callback); take(finished);
    finish_close(35,end); CHECK(atomic_load(&stop_count)==34);
    printf("BACKEND_TARGET_DRAIN_PASS\n");
    CHECK(port.io.open(port.io.ctx,fake_ap(),36));
    atomic_store(&fail_stop,true); end=deadline();
    for(unsigned i=0;i<100u && !backend.failed;++i) {
        CHECK(!port.close_until(port.io.ctx,36,end));
        vTaskDelay(1);
    }
    CHECK(backend.failed && atomic_load(&stop_count)==35);
    CHECK(!port.io.open(port.io.ctx,fake_ap(),37));
    printf("BACKEND_TARGET_QUARANTINE_PASS\n");
    printf("BACKEND_TARGET_PASS\n"); fflush(stdout);
}
