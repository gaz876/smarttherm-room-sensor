#include "room_setup_lifecycle.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    room_setup_lifecycle s={0};
    assert(!setup_life_open(&s,0,0));
    assert(setup_life_open(&s,0,1));
    assert(!setup_life_open(&s,1,2));
    assert(!setup_life_begin(&s,2,2));
    uint64_t t=setup_life_begin(&s,3,1); assert(t==1&&s.deadline==2003);
    assert(!setup_life_begin(&s,4,1));
    assert(!setup_life_sent(&s,5,1,t+1,true));
    assert(!setup_life_sent(&s,6,2,t,true));
    assert(setup_life_sent(&s,7,1,t,true)&&s.phase==SETUP_OPEN);
    t=setup_life_begin(&s,8,1); assert(t==2);
    assert(setup_life_final(&s,9,1,t)&&s.deadline==2008);
    assert(!setup_life_final(&s,10,1,t));
    assert(!setup_life_begin(&s,11,1));
    assert(!setup_life_drained(&s,12,1));
    setup_life_lost(&s,13,2); assert(s.phase==SETUP_FINAL);
    assert(setup_life_sent(&s,14,1,t,true)&&s.phase==SETUP_STOP);
    assert(!setup_life_open(&s,15,2));
    assert(!setup_life_drained(&s,16,2));
    assert(setup_life_drained(&s,17,1));
    assert(!setup_life_open(&s,18,1));
    assert(setup_life_open(&s,19,2));
    t=setup_life_begin(&s,20,2); assert(t==3);
    assert(!setup_life_sent(&s,21,1,2,true));
    assert(setup_life_final(&s,2019,2,t));
    assert(!setup_life_sent(&s,2020,2,t,true)&&s.phase==SETUP_STOP);
    assert(setup_life_drained(&s,2021,2));
    assert(setup_life_open(&s,2022,3));
    t=setup_life_begin(&s,2023,3); assert(t==4);
    assert(setup_life_sent(&s,2024,3,t,false)&&s.phase==SETUP_STOP);
    assert(setup_life_drained(&s,2025,3));
    assert(setup_life_open(&s,2026,4));
    t=setup_life_begin(&s,2027,4); assert(t==5);
    setup_life_lost(&s,2028,4); assert(s.phase==SETUP_STOP);
    assert(!setup_life_sent(&s,2029,4,t,true));
    assert(setup_life_drained(&s,2030,4));
    assert(setup_life_open(&s,2031,5));
    t=setup_life_begin(&s,2032,5); assert(t==6);
    setup_life_tick(&s,4032); assert(s.phase==SETUP_STOP);
    assert(setup_life_drained(&s,4033,5));
    assert(setup_life_open(&s,4034,6));
    setup_life_tick(&s,4033); assert(s.failed&&s.phase==SETUP_STOP);
    assert(setup_life_drained(&s,4035,6));
    assert(!setup_life_open(&s,4036,7));
    room_setup_lifecycle edge={0};
    assert(setup_life_open(&edge,UINT64_MAX-2,1));
    t=setup_life_begin(&edge,UINT64_MAX-1,1); assert(t&&edge.deadline==UINT64_MAX);
    setup_life_tick(&edge,UINT64_MAX); assert(edge.phase==SETUP_STOP);
    room_setup_lifecycle exhausted={.ticket=UINT64_MAX};
    assert(setup_life_open(&exhausted,0,1));
    assert(!setup_life_begin(&exhausted,1,1)&&exhausted.failed&&exhausted.phase==SETUP_STOP);
    puts("room setup lifecycle PASS"); return 0;
}
