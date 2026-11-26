#include "room_wifi_policy.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    room_wifi_policy p;
    assert(wifi_boot(&p,ROOM_WIFI_PROFILE_MISSING,0,false)==ROOM_WIFI_NONE && p.state==ROOM_WIFI_OFF);
    assert(wifi_tick(&p,999999)==ROOM_WIFI_NONE);
    assert(wifi_boot(&p,ROOM_WIFI_PROFILE_ERROR,0,true)==ROOM_WIFI_NONE && p.state==ROOM_WIFI_OFF);
    assert(wifi_manual(&p,100,false)==ROOM_WIFI_NONE && p.state==ROOM_WIFI_OFF);
    assert(wifi_boot(&p,ROOM_WIFI_PROFILE_MISSING,0,true)==ROOM_WIFI_START_SETUP);
    assert(wifi_manual(&p,100,true)==ROOM_WIFI_NONE && p.deadline==300000);
    assert(wifi_setup_done(&p,100,false)==ROOM_WIFI_NONE && p.state==ROOM_WIFI_SETUP);
    wifi_failed(&p,100);wifi_got_ip(&p);assert(p.state==ROOM_WIFI_SETUP);
    assert(wifi_tick(&p,299999)==ROOM_WIFI_NONE);
    assert(wifi_tick(&p,300000)==ROOM_WIFI_STOP && p.state==ROOM_WIFI_OFF);
    assert(wifi_manual(&p,300001,true)==ROOM_WIFI_START_SETUP);
    assert(wifi_setup_done(&p,300002,true)==ROOM_WIFI_STOP && p.profile==ROOM_WIFI_PROFILE_VALID);
    assert(wifi_tick(&p,300002)==ROOM_WIFI_CONNECT && p.state==ROOM_WIFI_CONNECTING);
    assert(wifi_tick(&p,330001)==ROOM_WIFI_NONE);
    assert(wifi_tick(&p,330002)==ROOM_WIFI_STOP && p.state==ROOM_WIFI_RETRY);
    assert(wifi_tick(&p,331001)==ROOM_WIFI_NONE);
    assert(wifi_tick(&p,331002)==ROOM_WIFI_CONNECT);
    wifi_got_ip(&p);assert(p.state==ROOM_WIFI_ONLINE && p.retry_ms==1000);
    wifi_failed(&p,332000); assert(p.deadline==333000 && p.state==ROOM_WIFI_RETRY);
    wifi_failed(&p,332001);wifi_got_ip(&p);assert(p.deadline==333000 && p.state==ROOM_WIFI_RETRY);
    for(unsigned i=0;i<10000;i++) {
        uint64_t t=p.deadline;
        assert(wifi_tick(&p,t-1)==ROOM_WIFI_NONE);
        assert(wifi_tick(&p,t)==ROOM_WIFI_CONNECT);
        wifi_failed(&p,t);assert(p.state==ROOM_WIFI_RETRY && p.deadline>t && p.retry_ms<=60000);
    }
    assert(p.retry_ms==60000);
    assert(wifi_manual(&p,p.deadline,true)==ROOM_WIFI_START_SETUP);
    assert(wifi_tick(&p,p.deadline)==ROOM_WIFI_STOP && p.state==ROOM_WIFI_RETRY);
    assert(wifi_tick(&p,p.deadline)==ROOM_WIFI_CONNECT);
    wifi_got_ip(&p);assert(p.state==ROOM_WIFI_ONLINE && p.retry_ms==1000);
    assert(wifi_boot(&p,ROOM_WIFI_PROFILE_VALID,0,true)==ROOM_WIFI_NONE && p.state==ROOM_WIFI_RETRY);
    assert(wifi_tick(&p,0)==ROOM_WIFI_CONNECT);
    assert(wifi_deadline(UINT64_MAX-1,1000)==UINT64_MAX);
    puts("PASS: missing/corrupt profile, setup gate/timeout, persistence gate, DHCP timeout, indefinite capped retry, manual setup, stale events, saturated deadline");
}
