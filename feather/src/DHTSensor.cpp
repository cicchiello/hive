#include <DHTSensor.h>

#include <Arduino.h>

#define P(args) Serial.print(args)
#define PL(args) Serial.println(args)


//#define NDEBUG
#ifndef NDEBUG
#define D(args) P(args)
#define DL(args) PL(args)
#else
#define D(args)
#define DL(args)
#endif

#include <str.h>


void DHTSensor::enqueueRequest(const char *value, const char *timestamp)
{
    enqueueFullRequest("temp", value, timestamp);
}

void DHTSensor::sensorSample(Str *value)
{
    *value = "3.43";
    P("Measured ambient temp: ");
    PL(value->c_str());
}
