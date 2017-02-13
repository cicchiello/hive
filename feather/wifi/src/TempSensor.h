#ifndef TempSensor_h
#define TempSensor_h


#include <SensorBase.h>

class Str;


class TempSensor : public SensorBase {
 public:
    TempSensor(const HiveConfig &config,
	       const char *name,
	       const class RateProvider &rateProvider,
	       const class TimeProvider &timeProvider,
	       unsigned long now);
    ~TempSensor();

    bool hasTemp() const {return mHasTemp;}
    double getTemp() const {return mT;}
    
    bool isItTimeYet(unsigned long now);

    bool sensorSample(Str *value);
    
 private:
    const char *className() const {return "TempSensor";}
    
    Str *mTempStr;
    double mT;
    bool mHasTemp;

    unsigned long mNextSampleTime, mNextPostTime;
    
    friend class HumidSensor;
    static class DHT &getDht();
};


#endif
