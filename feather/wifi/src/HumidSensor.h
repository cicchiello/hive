#ifndef HumidSensor_h
#define HumidSensor_h


#include <Sensor.h>

class Str;
class Mutex;
class HiveConfig;


class HumidSensor : public Sensor {
 public:
    HumidSensor(const HiveConfig &config, const char *name,
		const class RateProvider &rateProvider,
		const class TimeProvider &timeProvider,
		unsigned long now);
    ~HumidSensor();

    bool isItTimeYet(unsigned long now);
    bool loop(unsigned long now, Mutex *wifi);
    
    bool sensorSample(Str *value);
    
 private:
    const char *className() const {return "HumidSensor";}
    
    Str *mHumidStr;

    unsigned long mNextSampleTime, mNextPostTime;
    
    const HiveConfig &mConfig;
    class HttpCouchPost *mPoster;
};

#endif
