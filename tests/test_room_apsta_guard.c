/* Full radio-owner header using the existing synthetic SDK fixture. */
#define main original_apsta_fixture_main
#include "test_room_apsta_owner.c"
#undef main
static unsigned checks,guard_calls,fail_at;
#define CHECK(x) do { ++checks; assert(x); } while(0)
static bool guard(void *v) { (void)v;return ++guard_calls!=fail_at; }
int main(void) {
    for (unsigned i=1;i<=11;++i) {
        room_apsta_owner o={0};reset();guard_calls=0;fail_at=i;
        CHECK(START(1));pre=post=true;
        bool ok=room_apsta_stop_guarded(&o,1,true,barrier,NULL,guard,NULL);
        CHECK(ok==(i==11));CHECK(guard_calls==(i==11?10:i));
        if (i!=11) {
            CHECK(o.phase==APSTA_LOCKED);unsigned old_calls=calls,old_destroys=destroys;
            CHECK(!room_apsta_stop_guarded(&o,1,true,barrier,NULL,guard,NULL));
            CHECK(!START(2));CHECK(calls==old_calls&&destroys==old_destroys);
        } else { CHECK(o.phase==APSTA_IDLE&&!driver&&destroys==2&&deinits==1); }
    }
    printf("PASS: full APSTA guarded teardown: %u checks\n",checks);return 0;
}
