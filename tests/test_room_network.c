#include "room_network.h"
#include "room_wifi_secure_store.h"
#include "room_wifi_idf.h"
#include "mock.h"
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
static int scenario,creates,inits,loads,stations,steps,deletes;
static void (*task)(void *);
static jmp_buf done;
int xTaskCreate(void (*fn)(void *),const char *name,unsigned stack,void *arg,unsigned prio,void *handle) {
    assert(fn && !strcmp(name,"room_net") && stack==6144 && !arg && prio==4 && !handle);
    creates++; task=fn; return scenario==2?0:pdPASS;
}
bool room_secure_init(unsigned key) { assert(key==2); inits++; return scenario!=3; }
room_secure_status room_secure_load(room_enroll_profile *p) {
    loads++; *p=(room_enroll_profile){.ssid_len=1,.pass_len=8,.ssid={'x'},.pass={'a','b','c','d','e','f','g','h'}};
    return scenario==4?ROOM_SEC_MISSING:scenario==5?ROOM_SEC_ERROR:ROOM_SEC_VALID;
}
bool room_wifi_idf_init(const uint8_t *s,size_t n,const uint8_t *p,size_t m) {
    assert(n==1 && m==8 && s[0]=='x' && p[7]=='h'); stations++; return scenario!=6;
}
bool room_wifi_idf_step(bool *manual) { assert(!manual); steps++; return false; }
void vTaskDelay(TickType_t t) { assert(t==5); if(steps==3) longjmp(done,1); }
void vTaskDelete(void *h) { assert(!h); deletes++; }
int main(int argc,char **argv) {
    assert(argc==2); scenario=atoi(argv[1]);
    if(scenario==0) { assert(room_network_start(-1)); assert(!creates); }
    else if(scenario==1) { assert(!room_network_start(6)); assert(!room_network_start(-2)); assert(!creates); }
    else if(scenario==2) { assert(!room_network_start(2)); assert(creates==1&&!inits); }
    else {
        assert(room_network_start(2)); assert(!room_network_start(2)); assert(creates==1 && task);
        if(!setjmp(done)) task(NULL);
        assert(inits==1 && loads==(scenario!=3) && stations==(scenario>=6));
        assert(deletes==(scenario!=7) && steps==(scenario==7?3:0));
    }
    printf("network startup case %d PASS\n",scenario); return 0;
}
