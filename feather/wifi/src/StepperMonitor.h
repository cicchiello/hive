#ifndef StepperMonitor_h
#define StepperMonitor_h


#include <SensorBase.h>
#include <str.h>

class StepperMonitor : public SensorBase {
 public:
    StepperMonitor(const HiveConfig &config,
		   const char *name,
		   const class RateProvider &rateProvider,
		   const class TimeProvider &timeProvider,
		   unsigned long now,
		   const class StepperActuator &actuator,
		   Mutex *wifiMutex);
    ~StepperMonitor();

    bool isItTimeYet(unsigned long now);
    bool loop(unsigned long now);
    
    bool sensorSample(Str *value);
    
    bool processResult(const HttpCouchConsumer &consumer, unsigned long *callMeBackIn_ms);
    
 private:
    const char *className() const {return "StepperMonitor";}

    const class StepperActuator &mActuator;
    int mPrevTarget;
    Str mSensorValue;
    unsigned long mNextAction;
    bool mDoPost;
};


#endif
