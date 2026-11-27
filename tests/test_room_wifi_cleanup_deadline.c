/* Reuse the existing offline SDK fixture; each mode runs in a fresh process.
 * No real radio, credentials, event loop or hardware. */
#define main original_wifi_fixture_main
#include "test_room_wifi_idf.c"
#undef main
static uint64_t observed_deadline;
static bool late_return;
static bool timed_stop(void *v,uint64_t deadline,room_wifi_setup_result *result) {
    if (!observed_deadline) observed_deadline=deadline;
    assert(observed_deadline==deadline);
    bool ok=stop_setup(v,deadline,result);
    if (ok && late_return) clock_ms=deadline;
    return ok;
}
static void enter_setup(void) {
    room_wifi_setup_io io={NULL,bind_setup,ready_setup,start_setup,poll_setup,timed_stop,load_profile};
    assert(room_wifi_idf_bind_setup(io));
    assert(!step(0)&&starts==1&&profile_loads==1);
    linked=true;ni.up=true;ni.ip=1;assert(step(50));
    down=true;assert(step(100));assert(step(150));
    assert(!step(3150)&&queued&&stops==1);deliver();
    assert(!step(4150)&&queued&&deinits==1);deliver();
    assert(!step(5150)&&setup_running&&setup_starts==1&&!initialized);
    setup_poll_ok=false;
    assert(!step(6050)&&setup_running&&setup_stops==1&&observed_deadline==11050);
}
int main(int argc,char **argv) {
    assert(argc==2);
    assert(room_wifi_idf_init((const uint8_t *)"synthetic",9,(const uint8_t *)"test-only",9));
    if (!strcmp(argv[1],"sta-expiry")) {
        assert(!step(0)&&starts==1);linked=true;ni.up=true;ni.ip=1;assert(step(50));
        linked=false;assert(!step(100)&&stops==1&&queued);
        deliver();assert(!step(1100)&&deinits==1&&queued);deliver();
        assert(!step(5100)&&starts==1&&!initialized);
        assert(!step(6100)&&starts==1);puts("PASS: STA final acknowledgement cannot grant late handoff");return 0;
    }
    enter_setup();
    if (!strcmp(argv[1],"same-deadline")) {
        assert(!step(7050)&&setup_stops==2&&observed_deadline==11050);
        setup_stoppable=true;assert(!step(8050)&&setup_stops==3&&starts==2);
        assert(!setup_running&&profile_loads==2);puts("PASS: owner passes same deadline through retries");return 0;
    }
    if (!strcmp(argv[1],"late-return")) {
        setup_stoppable=true;late_return=true;
        assert(!step(7050)&&setup_stops==2&&!setup_running&&clock_ms==11050);
        assert(starts==1&&profile_loads==1&&!initialized);
        assert(!step(12050)&&starts==1&&setup_stops==2);puts("PASS: backend success at deadline is quarantined");return 0;
    }
    assert(!strcmp(argv[1],"expired-before-call"));
    setup_stoppable=true;assert(!step(11050)&&setup_stops==1&&setup_running);
    assert(!step(12050)&&starts==1&&setup_stops==1&&!initialized);
    puts("PASS: expired owner never calls backend again");return 0;
}
