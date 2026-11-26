#include "room_measure.h"
#include "room_i2c_retry.h"
#include "room_board.h"
#include "room_runtime.h"
#include "room_settings_nvs.h"
#include "driver/i2c_master.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
_Static_assert(ROOM_I2C_PINS_VALID(CONFIG_ROOM_I2C_SDA, CONFIG_ROOM_I2C_SCL), "invalid I2C pin mapping");
static const char *TAG="room_sensor";
_Static_assert(ROOM_SETTINGS_MIN_MS==ROOM_MIN_INTERVAL_MS && ROOM_SETTINGS_MAX_MS==ROOM_MAX_INTERVAL_MS, "settings interval mismatch");
static portMUX_TYPE snapshot_lock=portMUX_INITIALIZER_UNLOCKED;
static room_measure published;
static room_settings_result published_settings=ROOM_SETTINGS_IO_ERROR;
static void publish_snapshot(const room_measure *m, room_settings_result status) {
    portENTER_CRITICAL(&snapshot_lock);
    published=*m;
    published_settings=status;
    portEXIT_CRITICAL(&snapshot_lock);
}
room_status room_runtime_snapshot(sht40_sample *out, room_settings_result *settings_status) {
    room_measure copy;
    portENTER_CRITICAL(&snapshot_lock);
    copy=published;
    if (settings_status) *settings_status=published_settings;
    portEXIT_CRITICAL(&snapshot_lock);
    return room_snapshot(&copy,(uint64_t)(esp_timer_get_time()/1000),out);
}
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
typedef struct {
    i2c_master_bus_config_t cfg;
    i2c_device_config_t device;
    i2c_master_bus_handle_t bus;
    i2c_master_dev_handle_t dev;
} sensor_i2c;
static bool checked(esp_err_t err, const char *operation) {
    if (err==ESP_OK) return true;
    ESP_LOGW(TAG,"I2C %s failed: %d",operation,(int)err);
    return false;
}
static bool create_bus(void *ctx) {
    sensor_i2c *s=ctx;
    i2c_master_bus_handle_t bus=NULL;
    if (!checked(i2c_new_master_bus(&s->cfg,&bus),"create bus")) return false;
    s->bus=bus;
    return true;
}
static bool add_device(void *ctx) {
    sensor_i2c *s=ctx;
    i2c_master_dev_handle_t dev=NULL;
    if (!checked(i2c_master_bus_add_device(s->bus,&s->device,&dev),"add device")) return false;
    s->dev=dev;
    return true;
}
static void reset_bus(void *ctx) {
    sensor_i2c *s=ctx;
    (void)checked(i2c_master_bus_reset(s->bus),"reset bus");
}
static bool remove_device(void *ctx) {
    sensor_i2c *s=ctx;
    if (!checked(i2c_master_bus_rm_device(s->dev),"remove device")) return false;
    s->dev=NULL;
    return true;
}
static bool delete_bus(void *ctx) {
    sensor_i2c *s=ctx;
    if (!checked(i2c_del_master_bus(s->bus),"delete bus")) return false;
    s->bus=NULL;
    return true;
}
void app_main(void) {
    room_measure m;
    const room_settings defaults={CONFIG_ROOM_SAMPLE_INTERVAL_S*1000u,0};
    room_settings settings=defaults;
    room_settings_result settings_status=ROOM_SETTINGS_IO_ERROR;
    esp_err_t nvs_status=nvs_flash_init();
    if (nvs_status==ESP_OK) {
        const room_settings_store store=room_nvs_store();
        settings_status=room_settings_load(&store,defaults,&settings);
    } else {
        ESP_LOGW(TAG,"NVS init failed: %d; retained partition",(int)nvs_status);
    }
    ESP_LOGI(TAG,"Measurement settings status=%d interval=%lu ms correction=%ld mdeg",
        (int)settings_status,(unsigned long)settings.interval_ms,(long)settings.correction_mdeg);
    room_init(&m);
    if (!room_configure(&m,settings.interval_ms,settings.correction_mdeg/1000.0) ||
        !ROOM_I2C_PINS_VALID(CONFIG_ROOM_I2C_SDA, CONFIG_ROOM_I2C_SCL)) { ESP_LOGE(TAG,"Invalid configuration"); return; }
    publish_snapshot(&m,settings_status);
    sensor_i2c sensor={
        .cfg={.i2c_port=I2C_NUM_0,.sda_io_num=CONFIG_ROOM_I2C_SDA,
            .scl_io_num=CONFIG_ROOM_I2C_SCL,.clk_source=I2C_CLK_SRC_DEFAULT,.glitch_ignore_cnt=7},
        .device={.dev_addr_length=I2C_ADDR_BIT_LEN_7,.device_address=0x44,.scl_speed_hz=100000}};
    room_i2c_retry retry={0};
    const room_i2c_ops ops={&sensor,create_bus,add_device,reset_bus,remove_device,delete_bus};
    room_bus io={NULL,tx,rx,wait_ms,now_ms};
    for (;;) {
        if (!retry.ready) {
            uint32_t delay=room_i2c_prepare(&retry,&ops);
            if (delay) { wait_ms(NULL,delay); continue; }
            room_init(&m);
            if (!room_configure(&m,settings.interval_ms,settings.correction_mdeg/1000.0)) return;
            publish_snapshot(&m,settings_status);
            io.ctx=sensor.dev;
            ESP_LOGI(TAG,"I2C ready; awaiting valid measurement");
        }
        bool attempted=room_poll(&m,&io);
        publish_snapshot(&m,settings_status);
        if (attempted) {
            sht40_sample s;
            room_status status=room_snapshot(&m,now_ms(NULL),&s);
            if (status==ROOM_OK) ESP_LOGI(TAG,"T=%.2f C RH=%.2f %%",s.temperature,s.humidity);
            else ESP_LOGW(TAG,"No valid measurement: %d",(int)status);
            uint32_t delay=room_i2c_observe(&retry,status==ROOM_OK,status==ROOM_IO_ERROR);
            if (delay) {
                io.ctx=NULL;
                ESP_LOGW(TAG,"I2C recovery scheduled");
                wait_ms(NULL,delay);
                continue;
            }
        }
        wait_ms(NULL,50);
    }
}
