#include "sht40.h"
#include <math.h>
uint8_t sht40_crc(uint8_t a, uint8_t b) {
    uint8_t crc=0xff;
    uint8_t bytes[2]={a,b};
    for (unsigned i=0;i<2;i++) {
        crc ^= bytes[i];
        for (unsigned j=0;j<8;j++) crc=(uint8_t)((crc<<1)^((crc&0x80)?0x31:0));
    }
    return crc;
}
bool sht40_decode(const uint8_t *f,size_t len,double dt,double dh,uint64_t now,sht40_sample *out) {
    if (!out) return false;
    *out=(sht40_sample){0};
    if (!f || len!=6 || !isfinite(dt) || !isfinite(dh) || fabs(dt)>10 || fabs(dh)>30) return false;
    if (sht40_crc(f[0],f[1])!=f[2] || sht40_crc(f[3],f[4])!=f[5]) return false;
    double t=-45.0+175.0*(256u*f[0]+f[1])/65535.0;
    double h=-6.0+125.0*(256u*f[3]+f[4])/65535.0;
    if (t< -40 || t>125 || t+dt< -40 || t+dt>125) return false;
    h=fmin(100,fmax(0,h)); h=fmin(100,fmax(0,h+dh));
    *out=(sht40_sample){t+dt,h,now,true};
    return true;
}
bool sht40_fresh(const sht40_sample *s,uint64_t now,uint64_t max_age) {
    return s && s->valid && now>=s->measured_ms && now-s->measured_ms<=max_age;
}
