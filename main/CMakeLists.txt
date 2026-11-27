idf_component_register(SRCS "main.c" "room_apsta_session.c" "room_apsta_events.c" "room_apsta_task.c" "room_sec2_transport.c" "room_wifi_idf.c" "room_wifi_apsta_link.c" "room_wifi_identity_provider.c" "room_phone_channel.c" "room_sdk_stop.c" "room_network.c" "room_wifi_secure_store.c" "../src/sht40.c" "../src/room_measure.c"
    INCLUDE_DIRS "../src" REQUIRES esp_driver_i2c esp_timer freertos log nvs_flash esp_driver_gpio esp_wifi esp_netif esp_event efuse esp_partition nvs_sec_provider protocomm esp_http_server)

# Experimental footprint only: retain the uncalled API in the link map.
# No app_main/startup call, network listener or credential provisioner.
target_link_libraries(${COMPONENT_LIB} INTERFACE
    "-Wl,-u,room_sec2_transport_start"
    "-Wl,-u,room_sec2_transport_exchange"
    "-Wl,-u,room_sec2_transport_finish"
    "-Wl,-u,room_sec2_transport_destroy"
    "-Wl,-u,room_apsta_task_bind"
    "-Wl,-u,room_apsta_task_open"
    "-Wl,-u,room_apsta_task_poll"
    "-Wl,-u,room_apsta_task_packet"
    "-Wl,-u,room_apsta_task_sent"
    "-Wl,-u,room_apsta_task_close"
    "-Wl,-u,room_apsta_events_unbind"
    "-Wl,-u,room_wifi_apsta_link_bind"
    "-Wl,-u,room_wifi_apsta_link_init_missing"
    "-Wl,-u,room_wifi_identity_provider_init"
    "-Wl,-u,room_wifi_apsta_link_attach_io"
    "-Wl,-u,room_sdk_stop_bind"
    "-Wl,-u,room_sdk_stop_request"
    "-Wl,-u,room_sdk_stop_poll")
