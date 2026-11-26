#include "room_settings.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    uint8_t bytes[ROOM_SETTINGS_BYTES]; size_t len;
    room_settings_result result; bool write_fail; unsigned writes;
} fake;
static room_settings_result rd(void *p,uint8_t *data,size_t *len) {
    fake *f=p;
    if (f->result!=ROOM_SETTINGS_OK) return f->result;
    if (f->len>*len) return ROOM_SETTINGS_UNSUPPORTED;
    memcpy(data,f->bytes,f->len); *len=f->len; return ROOM_SETTINGS_OK;
}
static bool wr(void *p,const uint8_t *data,size_t len) {
    fake *f=p; ++f->writes;
    if (f->write_fail) return false;
    assert(len==sizeof(f->bytes)); memcpy(f->bytes,data,len); f->len=len; f->result=ROOM_SETTINGS_OK; return true;
}
static bool eq(room_settings a,room_settings b) {
    return a.interval_ms==b.interval_ms && a.correction_mdeg==b.correction_mdeg;
}
int main(void) {
    room_settings defaults={10000,0}, custom={15000,-1250}, out={0};
    fake f={.result=ROOM_SETTINGS_MISSING}; room_settings_store s={&f,rd,wr};
    assert(room_settings_load(&s,defaults,&out)==ROOM_SETTINGS_MISSING && eq(out,defaults) && f.writes==0);
    assert(room_settings_save(&s,custom,false)==ROOM_SETTINGS_OK && f.writes==1);
    const uint8_t expected[]={ 'R','S',1,0,0x98,0x3a,0,0,0x2e,0x22,0,0 };
    assert(memcmp(f.bytes,expected,sizeof(expected))==0);
    assert(room_settings_load(&s,defaults,&out)==ROOM_SETTINGS_OK && eq(out,custom));
    assert(room_settings_save(&s,custom,false)==ROOM_SETTINGS_UNCHANGED && f.writes==1);
    for (int32_t correction=-10000;correction<=10000;correction++) {
        room_settings x={correction%2?1000u:3600000u,correction};
        uint8_t data[ROOM_SETTINGS_BYTES]; assert(room_settings_encode(x,data));
        assert(room_settings_decode(data,sizeof(data),&out)==ROOM_SETTINGS_OK && eq(out,x));
    }
    const room_settings invalid[]={{999,0},{3600001,0},{10000,10001},{10000,-10001},{0,INT32_MIN},{UINT32_MAX,INT32_MAX}};
    for(size_t i=0;i<sizeof(invalid)/sizeof(invalid[0]);i++) assert(room_settings_save(&s,invalid[i],false)==ROOM_SETTINGS_INVALID);
    assert(f.writes==1);
    f.bytes[2]=2; assert(room_settings_load(&s,defaults,&out)==ROOM_SETTINGS_UNSUPPORTED && eq(out,defaults));
    assert(room_settings_save(&s,custom,false)==ROOM_SETTINGS_UNSUPPORTED && f.writes==1);
    assert(room_settings_save(&s,defaults,true)==ROOM_SETTINGS_OK && f.writes==2);
    f.bytes[3]=1; assert(room_settings_load(&s,defaults,&out)==ROOM_SETTINGS_INVALID && eq(out,defaults));
    f.len=2; assert(room_settings_load(&s,defaults,&out)==ROOM_SETTINGS_INVALID && f.writes==2);
    f.len=sizeof(f.bytes)+1; assert(room_settings_load(&s,defaults,&out)==ROOM_SETTINGS_UNSUPPORTED);
    f.result=ROOM_SETTINGS_IO_ERROR; assert(room_settings_load(&s,defaults,&out)==ROOM_SETTINGS_IO_ERROR && eq(out,defaults));
    assert(room_settings_save(&s,custom,true)==ROOM_SETTINGS_IO_ERROR && f.writes==2);
    f.result=ROOM_SETTINGS_MISSING; f.write_fail=true;
    assert(room_settings_save(&s,custom,false)==ROOM_SETTINGS_IO_ERROR && f.writes==3);
    assert(room_settings_load(&s,defaults,&out)==ROOM_SETTINGS_MISSING && eq(out,defaults));
    f.write_fail=false; assert(room_settings_save(&s,custom,false)==ROOM_SETTINGS_OK);
    assert(room_settings_save(&s,defaults,true)==ROOM_SETTINGS_OK);
    assert(room_settings_load(&s,custom,&out)==ROOM_SETTINGS_OK && eq(out,defaults));
    puts("PASS: defaults, wire format, 20001 correction values, range, unsupported/corrupt data, read/write failure, no-op, reset");
}
