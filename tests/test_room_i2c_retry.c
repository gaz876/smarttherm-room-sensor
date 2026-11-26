#include "room_i2c_retry.h"
#include <assert.h>
#include <stdio.h>

typedef struct {
    bool bus, device;
    unsigned fail_create, fail_add, fail_remove, fail_delete;
    unsigned creates, adds, resets, removes, deletes;
} fake;
static bool create(void *p) { fake *f=p; assert(!f->bus && !f->device); ++f->creates; if(f->fail_create){--f->fail_create;return false;} f->bus=true;return true; }
static bool add(void *p) { fake *f=p; assert(f->bus && !f->device); ++f->adds; if(f->fail_add){--f->fail_add;return false;} f->device=true;return true; }
static void reset(void *p) { fake *f=p; assert(f->bus && f->device); ++f->resets; }
static bool remove_dev(void *p) { fake *f=p; assert(f->bus && f->device); ++f->removes; if(f->fail_remove){--f->fail_remove;return false;} f->device=false;return true; }
static bool delete_bus(void *p) { fake *f=p; assert(f->bus && !f->device); ++f->deletes; if(f->fail_delete){--f->fail_delete;return false;} f->bus=false;return true; }
static room_i2c_ops ops(fake *f) { return (room_i2c_ops){f,create,add,reset,remove_dev,delete_bus}; }
static void check(room_i2c_retry *s, fake *f) { assert(s->bus==f->bus && s->device==f->device); assert(!s->ready || (s->bus && s->device)); }
int main(void) {
    room_i2c_retry s={0}; fake f={.fail_create=8}; room_i2c_ops o=ops(&f);
    const uint32_t delays[]={1000,2000,4000,8000,16000,30000,30000,30000};
    for(unsigned i=0;i<8;i++){assert(room_i2c_prepare(&s,&o)==delays[i]);check(&s,&f);assert(!s.ready);}
    assert(room_i2c_prepare(&s,&o)==0);check(&s,&f);assert(s.ready && f.creates==9 && f.adds==1);
    assert(room_i2c_observe(&s,true,false)==0 && s.delay_ms==0);
    assert(room_i2c_observe(&s,false,true)==0);assert(room_i2c_observe(&s,false,false)==0);
    assert(room_i2c_observe(&s,false,true)==0);assert(room_i2c_observe(&s,false,true)==0);assert(s.ready);
    assert(room_i2c_observe(&s,false,true)==1000 && !s.ready);
    f.fail_remove=1; f.fail_delete=1; f.fail_add=1;
    assert(room_i2c_prepare(&s,&o)==2000);check(&s,&f);assert(f.device && f.deletes==0);
    assert(room_i2c_prepare(&s,&o)==4000);check(&s,&f);assert(!f.device && f.bus && f.creates==9);
    assert(room_i2c_prepare(&s,&o)==8000);check(&s,&f);assert(f.bus && !f.device && f.creates==10);
    assert(room_i2c_prepare(&s,&o)==0);check(&s,&f);assert(s.ready && f.creates==11 && f.resets==2);
    assert(room_i2c_observe(&s,false,true)==0);assert(room_i2c_observe(&s,false,true)==0);assert(room_i2c_observe(&s,false,true)==16000);
    assert(room_i2c_prepare(&s,&o)==0);assert(room_i2c_observe(&s,true,false)==0);
    for(unsigned i=0;i<1000;i++){
        assert(room_i2c_observe(&s,false,true)==0);assert(room_i2c_observe(&s,false,true)==0);assert(room_i2c_observe(&s,false,true)==1000);
        assert(room_i2c_prepare(&s,&o)==0);check(&s,&f);assert(room_i2c_observe(&s,true,false)==0);
    }
    s=(room_i2c_retry){0}; f=(fake){.fail_add=1};
    assert(room_i2c_prepare(&s,&o)==1000);check(&s,&f);assert(f.bus && !f.device);
    assert(room_i2c_prepare(&s,&o)==0);check(&s,&f);assert(f.deletes==1 && f.creates==2);
    puts("PASS: startup, capped backoff, partial init, cleanup failure, consecutive I/O, 1000 recoveries");
    return 0;
}
