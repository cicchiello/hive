#include <CpuTempSensor.h>

#include <Arduino.h>

#include "Adafruit_BluefruitLE_SPI.h"

#define HEADLESS

#ifndef HEADLESS
#define P(args) Serial.print(args)
#define PL(args) Serial.println(args)
#else
#define P(args) 
#define PL(args) 
#endif


#define NDEBUG

#ifndef NDEBUG
#define D(args) P(args)
#define DL(args) PL(args)
#else
#define D(args)
#define DL(args)
#endif

#include <str.h>


void CpuTempSensor::enqueueRequest(const char *value, const char *timestamp)
{
    const char *sensorName = "cputemp";
    enqueueFullRequest(sensorName, value, timestamp);
}


void CpuTempSensor::sensorSample(Str *value)
{
    // crudely simulate a sensor by taking the BLE module's temperature
    mBle.println("AT+HWGETDIETEMP");
    mBle.readline();
    Str temp(mBle.buffer);
    if (! mBle.waitForOK() ) {
        PL(F("Failed to send?"));
    }
    P("Measured cputemp: ");
    PL(temp.c_str());
    *value = temp.c_str();
}
