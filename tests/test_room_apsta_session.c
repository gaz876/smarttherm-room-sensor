#define main apsta_fixture_main
#include "test_room_apsta_owner.c"
#undef main
#define main sec2_fixture_main
#include "test_room_sec2_transport.c"
#undef main
#include "room_apsta_session.h"
int main(void) {
    room_apsta_session s={0};fake f={0};room_enroll_store store={&f,ready,write_record,read_record};
    const char salt[16]={0},verifier[384]={0};protocomm_security2_params_t id={salt,16,verifier,384};
    const uint8_t *out;size_t n;uint8_t hs[]={1},stage[29]={1,ROOM_SETUP_STAGE,1,8,'x','a','b','c','d','e','f','g','h'},save[26]={1,ROOM_SETUP_SAVE,0,0,0,0,0,0,0,1};
    reset();
    assert(!room_apsta_session_open(&s,clock_ms,&f,PHONE_PROFILE_MISSING,false,false,&store,ssid,1,key,16,&id,7));
    assert(!calls);
    assert(room_apsta_session_open(&s,clock_ms,&f,PHONE_PROFILE_MISSING,false,true,&store,ssid,1,key,16,&id,7));
    room_apsta_session_ip(&s,&sta,1);assert(!s.lease);
    assert(room_sec2_transport_exchange(&s.transport,true,hs,sizeof(hs),&out,&n));assert(room_sec2_transport_finish(&s.transport,true));
    f.now=1;assert(room_sec2_transport_exchange(&s.transport,false,stage,sizeof(stage),&out,&n));
    assert(room_apsta_session_step(&s,true)&&!connects);
    assert(room_sec2_transport_finish(&s.transport,true));assert(room_apsta_session_step(&s,false)&&!connects);
    assert(room_apsta_session_step(&s,true)&&connects==1);
    linked=true;sta.up=true;sta.ip=1;
    room_apsta_session_ip(&s,&ap,1);assert(!s.lease);
    assert(room_apsta_session_step(&s,true)&&s.bridge.phone.enrollment.state==ENROLL_TRIAL);
    room_apsta_session_ip(&s,&sta,2);assert(room_apsta_session_step(&s,true)&&s.bridge.phone.enrollment.state==ENROLL_TRIAL);
    room_apsta_session_ip(&s,&sta,1);dhcp_status=ESP_NETIF_DHCP_STOPPED;
    assert(room_apsta_session_step(&s,true)&&s.bridge.phone.enrollment.state==ENROLL_TRIAL&&!f.writes);
    dhcp_status=ESP_NETIF_DHCP_STARTED;dhcp_error=true;
    assert(room_apsta_session_step(&s,true)&&s.bridge.phone.enrollment.state==ENROLL_TRIAL&&!f.writes);
    dhcp_error=false;assert(room_apsta_session_step(&s,true)&&s.bridge.phone.enrollment.state==ENROLL_VERIFIED);
    f.now=101;assert(room_sec2_transport_exchange(&s.transport,false,save,sizeof(save),&out,&n)&&out[1]==ROOM_REPLY_SAVED&&f.writes==1);
    assert(room_apsta_session_step(&s,true)&&!stops);assert(room_sec2_transport_finish(&s.transport,true));
    assert(!room_apsta_session_close(&s,false,barrier,NULL)&&!stops&&s.bridge.window);
    assert(!room_apsta_session_close(&s,true,barrier,NULL)&&stops==1&&s.bridge.window);
    pre=true;assert(!room_apsta_session_close(&s,true,barrier,NULL)&&s.bridge.window&&destroys==2);
    post=true;assert(room_apsta_session_close(&s,true,barrier,NULL)&&!s.bridge.window&&!s.transport.bridge);
    reset();f.now=200;assert(room_apsta_session_open(&s,clock_ms,&f,PHONE_PROFILE_VALID,true,true,&store,ssid,1,key,16,&id,7));
    assert(room_sec2_transport_exchange(&s.transport,true,hs,sizeof(hs),&out,&n));
    f.now=2199;assert(room_apsta_session_step(&s,true));
    f.now=2200;assert(!room_apsta_session_step(&s,true)&&s.bridge.stopping&&s.transport.pending&&s.transport.reply==out);
    assert(!room_sec2_transport_finish(&s.transport,true)&&!s.transport.pending);
    assert(!room_apsta_session_close(&s,true,barrier,NULL));f.now=7200;
    assert(!room_apsta_session_close(&s,true,barrier,NULL)&&s.locked&&s.bridge.window);
    /* Release only the fake allocation after testing permanent quarantine. */
    protocomm_delete(s.transport.sec.pc);
    puts("APSTA session integration mock PASS");return 0;
}
