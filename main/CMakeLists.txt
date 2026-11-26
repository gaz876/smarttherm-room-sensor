idf_component_register(SRCS "main.c" "../src/sht40.c" "../src/room_measure.c"
    INCLUDE_DIRS "../src" REQUIRES esp_driver_i2c esp_timer freertos log nvs_flash)
