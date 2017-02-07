#ifndef HumidSensor_h
#define HumidSensor_h


#include <Sensor.h>

class Str;


class HumidSensor : public Sensor {
 public:

    HumidSensor(const char *name, const class RateProvider &rateProvider, unsigned long now);
    ~HumidSensor() {}

    bool sensorSample(Str *value);
    
 private:
    const char *className() const {return "HumidSensor";}
    
    Str *mPrev;
};


#endif
