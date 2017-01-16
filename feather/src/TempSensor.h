#ifndef TempSensor_h
#define TempSensor_h


#include <Sensor.h>

class Str;


class TempSensor : public Sensor {
 public:

    TempSensor(const char *name, const class RateProvider &rateProvider, unsigned long now);
    ~TempSensor();

    bool sensorSample(Str *value);
    
 private:
    Str *mPrev;

    friend class HumidSensor;
    static class DHT &getDht();
};


#endif
