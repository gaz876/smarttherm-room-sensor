#ifndef ROOM_MQTT_SAMPLE_H
#define ROOM_MQTT_SAMPLE_H
#include "room_measure.h"

/* Offline, owner-task-only serialization. No broker, network, credentials,
 * retained command, controller output or default startup integration. */
enum { ROOM_MQTT_VALUE_CAP=16, ROOM_MQTT_TOPIC_CAP=128 };
typedef struct {
    char temperature[ROOM_MQTT_VALUE_CAP], humidity[ROOM_MQTT_VALUE_CAP];
    uint64_t measured_ms, valid_until_ms; /* exclusive deadline */
    bool valid, smarttherm_compatible;
} room_mqtt_sample;
typedef enum { ROOM_MQTT_INDOOR, ROOM_MQTT_OUTDOOR } room_mqtt_source;
/* Builds a fresh snapshot, clears output on every failure. Caller must not
 * queue or replay it after the deadline, and must serialize in the owner. */
room_status room_mqtt_sample_prepare(const room_measure *,uint64_t,room_mqtt_sample *);
/* Expected public ArduinoHA topic; explicit local configuration only.
 * Each argument is a single restricted ASCII level, not a wildcard.
 * Actual controller version must be confirmed before use. */
bool room_mqtt_expected_topic(const char *,const char *,room_mqtt_source,
    char *,size_t);
#endif
