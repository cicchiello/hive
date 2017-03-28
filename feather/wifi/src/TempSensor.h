#ifndef TempSensor_h
#define TempSensor_h


#include <SensorBase.h>

class Str;


class TempSensor : public SensorBase {
 public:
    TempSensor(const HiveConfig &config,
	       const char *name,
	       const class RateProvider &rateProvider,
	       unsigned long now, Mutex *wifiMutex);
    ~TempSensor();

    bool hasTemp() const {return mHasTemp;}
    double getTemp() const {return mT;}
    
    bool sensorSample(Str *value);
    
 private:
    const char *className() const {return "TempSensor";}
    
    Str *mTempStr;
    double mT;
    bool mHasTemp;

    friend class HumidSensor;
    static class DHT &getDht();
};


#endif
