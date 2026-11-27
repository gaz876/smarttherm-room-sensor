/* Compile-only SDK capability probe. Never starts a server or Wi-Fi.
 * This is not an operational configuration or credential source. */
#include "esp_https_server.h"
#include "esp_netif.h"
#include <stddef.h>
#include <net/if.h>
_Static_assert(sizeof(((httpd_config_t *)0)->if_name)==sizeof(struct ifreq *),
               "SDK lacks interface-specific HTTP bind member");
_Static_assert(sizeof(((httpd_ssl_config_t *)0)->tls_handshake_timeout_ms)>0,
               "SDK lacks TLS handshake timeout");
_Static_assert(sizeof(((struct ifreq *)0)->ifr_name)>0,
               "SDK lacks interface name");
static esp_err_t (*const name_probe)(esp_netif_t *,char *)=esp_netif_get_netif_impl_name;
static esp_err_t (*const start_probe)(httpd_handle_t *,httpd_ssl_config_t *)=httpd_ssl_start;
static esp_err_t (*const stop_probe)(httpd_handle_t)=httpd_ssl_stop;
void app_main(void) {
    (void)name_probe;(void)start_probe;(void)stop_probe;
}
