#ifndef MOCK_SDK_H
#define MOCK_SDK_H
#include "nvs.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef void *i2c_master_bus_handle_t;
typedef void *i2c_master_dev_handle_t;
typedef struct { int i2c_port, sda_io_num, scl_io_num, clk_source, glitch_ignore_cnt; } i2c_master_bus_config_t;
typedef struct { int dev_addr_length, device_address, scl_speed_hz; } i2c_device_config_t;
#define I2C_NUM_0 0
#define I2C_CLK_SRC_DEFAULT 0
#define I2C_ADDR_BIT_LEN_7 0
esp_err_t i2c_master_transmit(i2c_master_dev_handle_t,const uint8_t *,size_t,int);
esp_err_t i2c_master_receive(i2c_master_dev_handle_t,uint8_t *,size_t,int);
esp_err_t i2c_new_master_bus(const i2c_master_bus_config_t *,i2c_master_bus_handle_t *);
esp_err_t i2c_master_bus_add_device(i2c_master_bus_handle_t,const i2c_device_config_t *,i2c_master_dev_handle_t *);
esp_err_t i2c_master_bus_reset(i2c_master_bus_handle_t);
esp_err_t i2c_master_bus_rm_device(i2c_master_dev_handle_t);
esp_err_t i2c_del_master_bus(i2c_master_bus_handle_t);
int64_t esp_timer_get_time(void);
esp_err_t nvs_flash_init(void);
void mock_log(const char *,const char *,...);
#define ESP_LOGW mock_log
#define ESP_LOGI mock_log
#define ESP_LOGE mock_log
typedef unsigned TickType_t;
typedef int portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED 0
#define configTICK_RATE_HZ 100
void portENTER_CRITICAL(portMUX_TYPE *);
void portEXIT_CRITICAL(portMUX_TYPE *);
void vTaskDelay(TickType_t);
#define CONFIG_ROOM_SAMPLE_INTERVAL_S 10
#define CONFIG_ROOM_I2C_SDA 4
#define CONFIG_ROOM_I2C_SCL 5
#endif
