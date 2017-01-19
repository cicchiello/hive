#ifndef cputempsensor_h
#define cputempsensor_h


#include <Sensor.h>

class Adafruit_BluefruitLE_SPI;
class Str;


class CpuTempSensor : public Sensor {
 public:
    CpuTempSensor(const char *name, const class RateProvider &rateProvider,
		  unsigned long now, Adafruit_BluefruitLE_SPI &ble);
    ~CpuTempSensor() {}

    bool sensorSample(Str *value);
    
 private:
    const char *className() const {return "CpuTempSensor";}
    
    Adafruit_BluefruitLE_SPI &mBle;
    Str *result;
    int mState;
};


#endif
