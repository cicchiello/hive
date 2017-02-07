#ifndef TempSensor_h
#define TempSensor_h


#include <Sensor.h>

class Str;
class Mutex;
class HiveConfig;


class TempSensor : public Sensor {
 public:
    TempSensor(const HiveConfig &config, const char *name,
	       const class RateProvider &rateProvider,
	       const class TimeProvider &timeProvider,
	       unsigned long now);
    ~TempSensor();

    bool hasTemp() const {return mHasTemp;}
    double getTemp() const {return mT;}
    
    bool isItTimeYet(unsigned long now);
    bool loop(unsigned long now, Mutex *wifi);

    bool sensorSample(Str *value);
    
 private:
    const char *className() const {return "TempSensor";}
    
    Str *mTempStr;
    double mT;
    bool mHasTemp;

    unsigned long mNextSampleTime, mNextPostTime;
    
    const HiveConfig &mConfig;
    class HttpCouchPost *mPoster;
    
    friend class HumidSensor;
    static class DHT &getDht();
};


#endif
