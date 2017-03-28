#ifndef HumidSensor_h
#define HumidSensor_h


#include <SensorBase.h>

class Str;


class HumidSensor : public SensorBase {
 public:
    HumidSensor(const HiveConfig &config,
		const char *name,
		const class RateProvider &rateProvider,
		unsigned long now, Mutex *wifiMutex);
    ~HumidSensor();

    bool sensorSample(Str *value);
    
 private:
    const char *className() const {return "HumidSensor";}
    
    Str *mHumidStr;
};

#endif
