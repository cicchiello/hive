#ifndef HumidSensor_h
#define HumidSensor_h


#include <Sensor.h>

class Adafruit_BluefruitLE_SPI;
class Str;


class HumidSensor : public Sensor {
 public:

    HumidSensor(const char *name, unsigned long now);
    ~HumidSensor() {}

    bool sensorSample(Str *value);
    
 private:
    Str *mPrev;
};


#endif
