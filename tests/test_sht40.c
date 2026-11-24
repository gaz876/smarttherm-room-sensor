#include "sht40.h"
#include <assert.h>
#include <math.h>
int main(void) {
    assert(sht40_crc(0xbe,0xef)==0x92);
    uint8_t f[]={0x80,0,0,0x80,0,0};
    f[2]=sht40_crc(f[0],f[1]); f[5]=sht40_crc(f[3],f[4]);
    sht40_sample s;
    assert(sht40_decode(f,6,0,100,&s));
    assert(fabs(s.temperature-42.501335)<0.001);
    assert(fabs(s.humidity-56.500954)<0.001);
    assert(sht40_fresh(&s,110,10));
    assert(!sht40_fresh(&s,111,10));
    assert(!sht40_fresh(&s,99,10));
    for(unsigned i=0;i<48;i++) {
        f[i/8]^=(uint8_t)(1u<<(i%8));
        assert(!sht40_decode(f,6,0,100,&s) && !s.valid);
        f[i/8]^=(uint8_t)(1u<<(i%8));
    }
    assert(!sht40_decode(f,5,0,0,&s));
    assert(!sht40_decode(f,6,NAN,0,&s));
    assert(!sht40_decode(f,6,11,0,&s));
    assert(sht40_decode(f,6,1,100,&s));
    assert(fabs(s.temperature-43.501335)<0.001);
    assert(fabs(s.humidity-56.500954)<0.001);
    return 0;
}
