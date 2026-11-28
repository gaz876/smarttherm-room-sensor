#include "room_mqtt_sample.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static room_measure fixture(double t,double h){
    room_measure m;
    room_init(&m);
    m.status=ROOM_OK;m.attempted=true;m.last_start_ms=100;
    m.last_complete_ms=112;m.sample=(sht40_sample){t,h,100,true};
    return m;
}
static void cleared(const room_mqtt_sample *s){
    assert(!s->valid&&!s->smarttherm_compatible&&!s->temperature[0]&&!s->humidity[0]);
    assert(!s->measured_ms&&!s->valid_until_ms);
}
int main(void){
    room_mqtt_sample out;
    room_measure m=fixture(22.125,48.5);
    assert(room_mqtt_sample_prepare(&m,112,&out)==ROOM_OK);
    assert(out.valid&&out.smarttherm_compatible&&!strcmp(out.temperature,"22.125")&&!strcmp(out.humidity,"48.500"));
    assert(out.measured_ms==100&&out.valid_until_ms==10100);
    assert(room_mqtt_sample_prepare(&m,10099,&out)==ROOM_OK);
    assert(room_mqtt_sample_prepare(&m,10100,&out)==ROOM_STALE);cleared(&out);
    assert(room_mqtt_sample_prepare(&m,111,&out)==ROOM_STALE);cleared(&out);
    assert(room_mqtt_sample_prepare(NULL,112,&out)==ROOM_NOT_READY);cleared(&out);
    assert(room_mqtt_sample_prepare(&m,112,NULL)==ROOM_INVALID);
    m=fixture(-0.0001,100.0);assert(room_mqtt_sample_prepare(&m,112,&out)==ROOM_OK);
    assert(!strcmp(out.temperature,"0.000")&&!strcmp(out.humidity,"100.000"));
    m=fixture(-3.125,0.0);assert(room_mqtt_sample_prepare(&m,112,&out)==ROOM_OK);
    assert(!strcmp(out.temperature,"-3.125")&&!strcmp(out.humidity,"0.000"));
    m=fixture(50.001,50.0);assert(room_mqtt_sample_prepare(&m,112,&out)==ROOM_OK);
    assert(out.valid&&!out.smarttherm_compatible);
    m=fixture(50.0,50.0);assert(room_mqtt_sample_prepare(&m,112,&out)==ROOM_OK&&out.smarttherm_compatible);
    m=fixture(125.0,100.0);assert(room_mqtt_sample_prepare(&m,112,&out)==ROOM_OK&&!out.smarttherm_compatible);
    const double bad_t[]={NAN,INFINITY,-40.001,125.001};
    const double bad_h[]={NAN,-INFINITY,-0.001,100.001};
    for(size_t i=0;i<sizeof bad_t/sizeof bad_t[0];++i){
        m=fixture(bad_t[i],50);assert(room_mqtt_sample_prepare(&m,112,&out)==ROOM_INVALID);cleared(&out);
        m=fixture(22,bad_h[i]);assert(room_mqtt_sample_prepare(&m,112,&out)==ROOM_INVALID);cleared(&out);
    }
    const room_status states[]={ROOM_NOT_READY,ROOM_IO_ERROR,ROOM_INVALID,ROOM_STALE};
    for(size_t i=0;i<sizeof states/sizeof states[0];++i){
        m=fixture(22,50);m.status=states[i];
        assert(room_mqtt_sample_prepare(&m,112,&out)==states[i]);cleared(&out);
    }
    m=fixture(22,50);m.sample.valid=false;
    assert(room_mqtt_sample_prepare(&m,112,&out)==ROOM_STALE);cleared(&out);
    m=fixture(22,50);m.last_start_ms=UINT64_MAX-10u;m.last_complete_ms=UINT64_MAX-1u;m.sample.measured_ms=m.last_start_ms;
    assert(room_mqtt_sample_prepare(&m,UINT64_MAX-1u,&out)==ROOM_INVALID);cleared(&out);
    char topic[ROOM_MQTT_TOPIC_CAP];
    assert(room_mqtt_expected_topic("aha","TEST",ROOM_MQTT_INDOOR,topic,sizeof topic));
    assert(!strcmp(topic,"aha/TEST/TEST_Tindoor/cmd_t"));
    assert(room_mqtt_expected_topic("aha","TEST",ROOM_MQTT_OUTDOOR,topic,sizeof topic));
    assert(!strcmp(topic,"aha/TEST/TEST_Toutdoor/cmd_t"));
    const char *bad[]={NULL,"","a/b","+","#","a b","a\n","abcdefghijklmnopqrstuvwxyz012345"};
    for(size_t i=0;i<sizeof bad/sizeof bad[0];++i){
        assert(!room_mqtt_expected_topic("aha",bad[i],ROOM_MQTT_INDOOR,topic,sizeof topic)&&!topic[0]);
        assert(!room_mqtt_expected_topic(bad[i],"TEST",ROOM_MQTT_OUTDOOR,topic,sizeof topic)&&!topic[0]);
    }
    assert(!room_mqtt_expected_topic("aha","TEST",(room_mqtt_source)99,topic,sizeof topic)&&!topic[0]);
    assert(!room_mqtt_expected_topic("aha","TEST",ROOM_MQTT_INDOOR,topic,5)&&!topic[0]);
    assert(!room_mqtt_expected_topic("aha","TEST",ROOM_MQTT_INDOOR,NULL,0));
    puts("MQTT_SAMPLE_OFFLINE_PASS");
    return 0;
}
