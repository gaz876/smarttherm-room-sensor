#include "room_network.h"
#include "room_network_owner.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#if defined(CONFIG_NVS_ENCRYPTION) || defined(CONFIG_SECURE_FLASH_ENC_ENABLED) || defined(CONFIG_SECURE_BOOT)
#error "This experimental build must not enable automatic security provisioning"
#endif
static bool launched;
static unsigned selected_key;
static void network_task(void *arg) {
    (void)arg;
    room_secure_status status;
    /* No qualified HTTP backend is available in the normal build yet. */
    bool station=room_network_owner_prepare(selected_key,NULL,&status);
    ESP_LOGI("room_network","Storage status=%d; station initialized=%d",(int)status,(int)station);
    /* Missing/corrupt profile is not authorization to open an AP. */
    if (!station) { vTaskDelete(NULL); return; }
    TickType_t ticks=pdMS_TO_TICKS(50);
    for (;;) {
        (void)room_wifi_idf_step(NULL);
        vTaskDelay(ticks?ticks:1);
    }
}
bool room_network_start(int key_id) {
    if (key_id<0) return key_id==-1;
    if (key_id>=6 || launched) return false;
    selected_key=(unsigned)key_id;
    launched=true;
    if (xTaskCreate(network_task,"room_net",6144,NULL,4,NULL)!=pdPASS) {
        launched=false;
        return false;
    }
    return true;
}
