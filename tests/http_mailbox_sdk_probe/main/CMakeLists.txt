idf_component_register(SRCS "probe.c" "../../../main/room_sdk_stop.c"
    INCLUDE_DIRS "../../../src" "../../browser_worker"
    REQUIRES freertos esp_timer esp_http_server esp_netif esp_event lwip)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
