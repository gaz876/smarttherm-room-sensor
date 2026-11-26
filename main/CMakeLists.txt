idf_component_register(SRCS "main.c" "room_wifi_idf.c" "room_network.c" "room_wifi_secure_store.c" "../src/sht40.c" "../src/room_measure.c"
    INCLUDE_DIRS "../src" REQUIRES esp_driver_i2c esp_timer freertos log nvs_flash esp_driver_gpio esp_wifi esp_netif esp_event efuse esp_partition nvs_sec_provider)
