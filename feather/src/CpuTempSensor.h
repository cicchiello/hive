#ifndef cputempsensor_h
#define cputempsensor_h


#include <Sensor.h>

class Adafruit_BluefruitLE_SPI;
class Str;


class CpuTempSensor : public Sensor {
 public:
    CpuTempSensor(const char *name, const class SensorRateActuator &rateProvider,
		  unsigned long now, Adafruit_BluefruitLE_SPI &ble);
    ~CpuTempSensor() {}

    bool sensorSample(Str *value);
    
 private:
    Adafruit_BluefruitLE_SPI &mBle;
    Str *result;
    int mState;
};


#endif
