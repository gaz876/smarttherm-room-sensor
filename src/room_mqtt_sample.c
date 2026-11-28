#include "room_mqtt_sample.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static bool decimal3(double v,char out[ROOM_MQTT_VALUE_CAP]){
    if(!isfinite(v)||v < -125.0||v > 125.0)return false;
    unsigned q=(unsigned)(fabs(v)*1000.0+0.5);
    int n=snprintf(out,ROOM_MQTT_VALUE_CAP,"%s%u.%03u",
        v<0.0&&q?"-":"",q/1000u,q%1000u);
    return n>0&&(size_t)n<ROOM_MQTT_VALUE_CAP;
}
room_status room_mqtt_sample_prepare(const room_measure *m,uint64_t now,room_mqtt_sample *out){
    if(!out)return ROOM_INVALID;
    *out=(room_mqtt_sample){0};
    sht40_sample s={0};
    room_status status=room_snapshot(m,now,&s);
    if(status!=ROOM_OK)return status;
    if(!s.valid||!isfinite(s.temperature)||!isfinite(s.humidity)||
        s.temperature < -40.0||s.temperature > 125.0||
        s.humidity < 0.0||s.humidity > 100.0||
        m->interval_ms < ROOM_MIN_INTERVAL_MS||m->interval_ms > ROOM_MAX_INTERVAL_MS||
        m->last_start_ms > UINT64_MAX-m->interval_ms)return ROOM_INVALID;
    room_mqtt_sample next={0};
    if(!decimal3(s.temperature,next.temperature)||!decimal3(s.humidity,next.humidity))
        return ROOM_INVALID;
    next.measured_ms=s.measured_ms;
    next.valid_until_ms=m->last_start_ms+m->interval_ms;
    if(now>=next.valid_until_ms)return ROOM_STALE;
    next.smarttherm_compatible=s.temperature>=-50.0&&s.temperature<=50.0;
    next.valid=true;
    *out=next;
    return ROOM_OK;
}
static bool level(const char *s){
    if(!s)return false;
    for(size_t i=0;i<32u;++i){
        unsigned char c=(unsigned char)s[i];
        if(!c)return i!=0;
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
             (c>='0'&&c<='9')||c=='_'||c=='-'||c=='.'))return false;
    }
    return false;
}
bool room_mqtt_expected_topic(const char *prefix,const char *id,room_mqtt_source source,
    char *out,size_t cap){
    if(!out||!cap)return false;
    out[0]=0;
    if(!level(prefix)||!level(id)||(source!=ROOM_MQTT_INDOOR&&source!=ROOM_MQTT_OUTDOOR))return false;
    int n=snprintf(out,cap,"%s/%s/%s_%s/cmd_t",prefix,id,id,
        source==ROOM_MQTT_INDOOR?"Tindoor":"Toutdoor");
    if(n<0||(size_t)n>=cap){out[0]=0;return false;}
    return true;
}
