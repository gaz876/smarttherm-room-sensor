idf_component_register(SRCS "probe.c" "../../../main/room_http_backend.c" "../../../main/room_http_idf_driver.c" "../../../main/room_http_idf_peer.c" "../../../main/room_sdk_stop.c"
    INCLUDE_DIRS "../../../main" "../../../src"
    REQUIRES freertos esp_timer esp_http_server esp_netif esp_event esp_wifi protocomm lwip heap)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,--wrap=esp_netif_get_netif_impl_name")
