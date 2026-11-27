idf_component_register(SRCS "probe.c" "../../../main/room_sdk_stop.c"
    INCLUDE_DIRS "../../../src" REQUIRES freertos esp_timer esp_http_server esp_netif lwip)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
