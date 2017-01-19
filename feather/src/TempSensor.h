#ifndef TempSensor_h
#define TempSensor_h


#include <Sensor.h>

class Str;


class TempSensor : public Sensor {
 public:

    TempSensor(const char *name, const class RateProvider &rateProvider, unsigned long now);
    ~TempSensor();

    bool hasTemp() const {return mHasTemp;}
    double getTemp() const {return mT;}
    
    bool sensorSample(Str *value);
    
 private:
    const char *className() const {return "TempSensor";}
    
    Str *mPrev;
    double mT;
    bool mHasTemp;

    friend class HumidSensor;
    static class DHT &getDht();
};


#endif
