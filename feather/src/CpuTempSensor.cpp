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

#define assert(c,msg) if (!(c)) {PL("ASSERT"); WDT_TRACE(msg); while(1);}


#include <platformutils.h>

#include <str.h>


CpuTempSensor::CpuTempSensor(unsigned long now, Adafruit_BluefruitLE_SPI &ble)
  : Sensor(now), mBle(ble), result(new Str()), mState(0)
{
}


void CpuTempSensor::enqueueRequest(const char *value, const char *timestamp)
{
    const char *sensorName = "cputemp";
    enqueueFullRequest(sensorName, value, timestamp);
}


bool CpuTempSensor::isMyResponse(const char *rsp) const
{
    if (mState > 0) {
      DL("CpuTempSensor::isMyResponse");
    }
    return false;
}


// crudely simulate a sensor by taking the BLE module's temperature
bool CpuTempSensor::sensorSample(Str *value)
{
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
