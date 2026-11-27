#define _POSIX_C_SOURCE 200809L
#include "room_admission.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stddef.h>
#include <semaphore.h>
#include <errno.h>
static unsigned checks;
#define CHECK(x) do { ++checks; assert(x); } while(0)
static void wait_sem(sem_t *s) { int rc;do{rc=sem_wait(s);}while(rc&&errno==EINTR);assert(!rc); }
typedef struct { room_admission gate;sem_t ready,release;unsigned payload[8]; } fixture;
static void *callback(void *v) {
    fixture *f=v;room_lease lease={0};assert(room_admission_enter(&f->gate,7,&lease));
    for(unsigned i=0;i<8;++i)f->payload[i]=i+1;
    assert(!sem_post(&f->ready));wait_sem(&f->release);
    for(unsigned i=0;i<8;++i)((volatile unsigned*)f->payload)[i]=0;
    assert(room_admission_leave(&lease));assert(!room_admission_leave(&lease));return NULL;
}
static void *race_enter(void *v) { room_admission *g=v;room_lease l={0};if(room_admission_enter(g,7,&l))assert(room_admission_leave(&l));return NULL; }
int main(void) {
    room_admission g;room_lease leases[ROOM_ADMISSION_MAX]={0},extra={0};
    CHECK(!room_admission_init(&g,0));CHECK(room_admission_init(&g,7));
    CHECK(!room_admission_enter(&g,6,&extra));CHECK(!room_admission_drained(&g,7));
    for(unsigned i=0;i<ROOM_ADMISSION_MAX;++i)CHECK(room_admission_enter(&g,7,&leases[i]));
    CHECK(!room_admission_enter(&g,7,&extra));CHECK(!room_admission_enter(&g,7,&leases[0]));
    CHECK(!room_admission_close(&g,8));CHECK(room_admission_close(&g,7));CHECK(room_admission_close(&g,7));
    CHECK(!room_admission_enter(&g,7,&extra));CHECK(!room_admission_drained(&g,7));
    for(unsigned i=0;i<ROOM_ADMISSION_MAX;++i){CHECK(room_admission_leave(&leases[i]));CHECK(!room_admission_leave(&leases[i]));CHECK(room_admission_drained(&g,7)==(i==ROOM_ADMISSION_MAX-1));}
    CHECK(!room_admission_drained(&g,8));
    fixture f={0};pthread_t worker;CHECK(room_admission_init(&f.gate,7));CHECK(!sem_init(&f.ready,0,0));CHECK(!sem_init(&f.release,0,0));
    CHECK(!pthread_create(&worker,NULL,callback,&f));wait_sem(&f.ready);
    CHECK(room_admission_enter(&f.gate,7,&extra));CHECK(room_admission_close(&f.gate,7));
    CHECK(!room_admission_drained(&f.gate,7));CHECK(room_admission_leave(&extra));CHECK(!room_admission_drained(&f.gate,7));
    CHECK(!sem_post(&f.release));while(!room_admission_drained(&f.gate,7)){};
    /* Acquire of the final release is the proof, not pthread_join. */
    for(unsigned i=0;i<8;++i){CHECK(!f.payload[i]);}CHECK(!pthread_join(worker,NULL));CHECK(!sem_destroy(&f.ready));CHECK(!sem_destroy(&f.release));
    for(unsigned n=0;n<100;++n){room_admission gate;pthread_t ts[4];CHECK(room_admission_init(&gate,7));for(unsigned i=0;i<4;++i)CHECK(!pthread_create(&ts[i],NULL,race_enter,&gate));CHECK(room_admission_close(&gate,7));CHECK(!room_admission_enter(&gate,7,&extra));for(unsigned i=0;i<4;++i)CHECK(!pthread_join(ts[i],NULL));CHECK(room_admission_drained(&gate,7));}
    printf("admission: %u checks PASS\n",checks);return 0;
}
