#ifndef HumidSensor_h
#define HumidSensor_h


#include <Sensor.h>

class Adafruit_BluefruitLE_SPI;
class Str;


class HumidSensor : public Sensor {
 public:

    HumidSensor(unsigned long now);
    ~HumidSensor() {}

    bool sensorSample(Str *value);
    
    void enqueueRequest(const char *value, const char *timestamp);

 private:
    Str *mPrev;
};


#endif
