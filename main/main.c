#include "room_measure.h"
#include "driver/i2c_master.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
static const char *TAG="room_sensor";
static bool tx(void *ctx,uint8_t cmd) {
    return i2c_master_transmit((i2c_master_dev_handle_t)ctx,&cmd,1,50)==ESP_OK;
}
static bool rx(void *ctx,uint8_t *data,size_t len) {
    return i2c_master_receive((i2c_master_dev_handle_t)ctx,data,len,50)==ESP_OK;
}
static uint64_t now_ms(void *ctx) { (void)ctx; return (uint64_t)(esp_timer_get_time()/1000); }
static void wait_ms(void *ctx,uint32_t ms) {
    (void)ctx;
    /* Round UP and add one tick to account for partial first tick. */
    const TickType_t ticks=(TickType_t)(((uint64_t)ms*configTICK_RATE_HZ+999)/1000+1);
    vTaskDelay(ticks);
}
void app_main(void) {
    room_measure m;
    room_init(&m);
    if (!room_configure(&m,CONFIG_ROOM_SAMPLE_INTERVAL_S*1000u,0.0) ||
        CONFIG_ROOM_I2C_SDA==CONFIG_ROOM_I2C_SCL) { ESP_LOGE(TAG,"Invalid configuration"); return; }
    i2c_master_bus_config_t cfg={.i2c_port=I2C_NUM_0,
        .sda_io_num=CONFIG_ROOM_I2C_SDA,.scl_io_num=CONFIG_ROOM_I2C_SCL,
        .clk_source=I2C_CLK_SRC_DEFAULT,.glitch_ignore_cnt=7};
    i2c_master_bus_handle_t bus=NULL;
    i2c_master_dev_handle_t dev=NULL;
    i2c_device_config_t device={.dev_addr_length=I2C_ADDR_BIT_LEN_7,
        .device_address=0x44,.scl_speed_hz=100000};
    if (i2c_new_master_bus(&cfg,&bus)!=ESP_OK) { ESP_LOGE(TAG,"I2C bus init failed"); return; }
    if (i2c_master_bus_add_device(bus,&device,&dev)!=ESP_OK) {
        ESP_LOGE(TAG,"I2C device init failed"); (void)i2c_del_master_bus(bus); return;
    }
    room_bus io={dev,tx,rx,wait_ms,now_ms};
    for (;;) {
        if (room_poll(&m,&io)) {
            sht40_sample s;
            room_status status=room_snapshot(&m,now_ms(NULL),&s);
            if (status==ROOM_OK) ESP_LOGI(TAG,"T=%.2f C RH=%.2f %%",s.temperature,s.humidity);
            else ESP_LOGW(TAG,"No valid measurement: %d",(int)status);
        }
        wait_ms(NULL,50);
    }
}
