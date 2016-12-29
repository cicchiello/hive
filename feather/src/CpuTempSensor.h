#ifndef cputempsensor_h
#define cputempsensor_h


#include <Sensor.h>

class Adafruit_BluefruitLE_SPI;
class Str;


class CpuTempSensor : public Sensor {
 public:
    CpuTempSensor(unsigned long now, Adafruit_BluefruitLE_SPI &ble);
    ~CpuTempSensor() {}

    bool sensorSample(Str *value);
    
    bool isMyResponse(const char *rsp) const;
    
    void enqueueRequest(const char *value, const char *timestamp);

 private:
    Adafruit_BluefruitLE_SPI &mBle;
    Str *result;
    int mState;
};


#endif
