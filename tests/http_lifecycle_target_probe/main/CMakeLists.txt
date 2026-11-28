idf_component_register(SRCS "../../test_room_http_lifecycle_worker.c" "../../../main/room_sdk_stop.c"
    INCLUDE_DIRS "../../../src" REQUIRES freertos esp_timer esp_http_server)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
