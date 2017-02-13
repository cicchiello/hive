#ifndef HumidSensor_h
#define HumidSensor_h


#include <SensorBase.h>

class Str;


class HumidSensor : public SensorBase {
 public:
    HumidSensor(const HiveConfig &config,
		const char *name,
		const class RateProvider &rateProvider,
		const class TimeProvider &timeProvider,
		unsigned long now);
    ~HumidSensor();

    bool isItTimeYet(unsigned long now);
    
    bool sensorSample(Str *value);
    
 private:
    const char *className() const {return "HumidSensor";}
    
    Str *mHumidStr;

    unsigned long mNextSampleTime, mNextPostTime;
};

#endif
