#include "room_http_idf_peer.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "lwip/sockets.h"
#include <string.h>

/* Read-only local SDK observations. No real listener is started here.
 * A caller must independently qualify AP-only socket binding and the epoch
 * source on the pinned target. DHCP membership alone is not authorization. */
bool room_http_idf_peer_observe(void *ctx,const esp_netif_t *ap,const httpd_req_t *r,room_http_peer_observation *out){
    if(!out)return false;
    *out=(room_http_peer_observation){0};
    room_http_idf_peer *p=ctx;
    if(!p||!p->epoch||!ap||!r)return false;
    esp_netif_t *netif=(esp_netif_t *)ap;
    wifi_sta_list_t list={0},after={0};
    esp_netif_ip_info_t ip={0};
    esp_netif_dhcp_status_t dhcp=ESP_NETIF_DHCP_INIT;
    if(!esp_netif_is_netif_up(netif)||esp_netif_get_ip_info(netif,&ip)!=ESP_OK||!ip.ip.addr||
       esp_netif_dhcps_get_status(netif,&dhcp)!=ESP_OK||dhcp!=ESP_NETIF_DHCP_STARTED||
       esp_wifi_ap_get_sta_list(&list)!=ESP_OK||list.num!=1)return false;
    esp_netif_pair_mac_ip_t pair={0};memcpy(pair.mac,list.sta[0].mac,sizeof(pair.mac));
    uint64_t before=0,final=0,peer=0;
    for(size_t i=0;i<sizeof(pair.mac);++i)peer=(peer<<8)|pair.mac[i];
    if(!peer||!p->epoch(p->ctx,ap,pair.mac,&before)||!before||
       esp_netif_dhcps_get_clients_by_mac(netif,1,&pair)!=ESP_OK||!pair.ip.addr||pair.ip.addr==ip.ip.addr)return false;
    /* httpd_req_to_sockfd reads the URI handler's live request. */
    int fd=httpd_req_to_sockfd((httpd_req_t *)r);
    struct sockaddr_in local={0},remote={0};
    socklen_t ln=sizeof(local),rn=sizeof(remote);
    if(fd<0||getsockname(fd,(struct sockaddr *)&local,&ln)!=0||
       getpeername(fd,(struct sockaddr *)&remote,&rn)!=0||ln!=sizeof(local)||rn!=sizeof(remote)||
       local.sin_family!=AF_INET||remote.sin_family!=AF_INET||
       local.sin_addr.s_addr!=ip.ip.addr||remote.sin_addr.s_addr!=pair.ip.addr)return false;
    if(!esp_netif_is_netif_up(netif)||esp_wifi_ap_get_sta_list(&after)!=ESP_OK||after.num!=1||
       memcmp(after.sta[0].mac,pair.mac,sizeof(pair.mac))||
       !p->epoch(p->ctx,ap,pair.mac,&final)||final!=before)return false;
    *out=(room_http_peer_observation){true,true,true,before,peer};return true;
}
