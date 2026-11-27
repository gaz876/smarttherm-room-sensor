#include "worker_model.h"
#include <assert.h>
#include <stdio.h>
static unsigned checks;
#define CHECK(x) do { assert(x); ++checks; } while (0)
int main(void) {
    worker_model w={0};
    CHECK(!worker_request(&w,0,1,1));
    CHECK(!worker_open(&w,0,0)); CHECK(worker_open(&w,0,1));
    CHECK(!worker_open(&w,1,2)); CHECK(!worker_started(&w,2,2,true));
    CHECK(worker_started(&w,3,1,true));
    CHECK(!worker_request(&w,4,1,0)); CHECK(!worker_request(&w,4,1,4097));
    CHECK(!worker_request(&w,4,2,1));
    uint64_t t=worker_request(&w,5,1,4096); CHECK(t==1);
    CHECK(!worker_request(&w,6,1,1)); CHECK(!worker_reply(&w,7,2,t,1));
    CHECK(!worker_reply(&w,8,1,t+1,1)); CHECK(worker_reply(&w,9,1,t,4096));
    CHECK(!worker_reply(&w,10,1,t,1)); CHECK(!worker_sent(&w,11,1,t+1,true));
    CHECK(worker_sent(&w,12,1,t,true)); CHECK(w.phase==W_IDLE);
    t=worker_request(&w,13,1,1); CHECK(t==2);
    CHECK(worker_reply(&w,2012,1,t,1));
    CHECK(!worker_sent(&w,2013,1,t,true)); CHECK(w.phase==W_STOP);
    CHECK(!worker_open(&w,2014,2)); CHECK(!worker_drained(&w,2015,1,true,false));
    CHECK(!worker_drained(&w,2016,1,false,true)); CHECK(!worker_drained(&w,2017,2,true,true));
    CHECK(worker_drained(&w,2018,1,true,true)); CHECK(!worker_open(&w,2019,1));
    CHECK(worker_open(&w,2020,2)); CHECK(!worker_started(&w,2021,2,false));
    CHECK(w.phase==W_STOP); CHECK(worker_drained(&w,2022,2,true,true));
    CHECK(worker_open(&w,2023,3)); CHECK(!worker_started(&w,4023,3,true));
    CHECK(w.phase==W_STOP); worker_stop(&w,4024); CHECK(w.stop_deadline==6023);
    CHECK(!worker_drained(&w,6023,3,true,true)); CHECK(w.phase==W_LOCKED);
    CHECK(!worker_open(&w,6024,4));
    w=(worker_model){0}; CHECK(worker_open(&w,0,1)); CHECK(worker_started(&w,1,1,true));
    t=worker_request(&w,2,1,1); CHECK(t==1); CHECK(!worker_reply(&w,3,1,t,0));
    CHECK(w.phase==W_STOP); CHECK(worker_drained(&w,4,1,true,true));
    CHECK(worker_open(&w,5,2)); CHECK(worker_started(&w,6,2,true));
    t=worker_request(&w,7,2,1); CHECK(t==2); CHECK(worker_reply(&w,8,2,t,1));
    CHECK(!worker_sent(&w,9,2,t,false)); CHECK(w.phase==W_STOP);
    CHECK(worker_drained(&w,10,2,true,true)); CHECK(worker_open(&w,11,3));
    worker_tick(&w,10); CHECK(w.fault && w.phase==W_STOP);
    CHECK(!worker_drained(&w,12,3,true,true));
    w=(worker_model){0}; CHECK(worker_open(&w,UINT64_MAX-1,1));
    CHECK(!worker_started(&w,UINT64_MAX,1,true)); CHECK(w.fault && w.phase==W_LOCKED);
    w=(worker_model){0}; CHECK(worker_open(&w,0,1)); CHECK(worker_started(&w,1,1,true));
    w.ticket=UINT64_MAX; CHECK(!worker_request(&w,2,1,1)); CHECK(w.fault && w.phase==W_STOP);
    printf("worker model: %u checks PASS\n",checks); return 0;
}
