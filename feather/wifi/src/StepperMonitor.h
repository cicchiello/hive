#ifndef StepperMonitor_h
#define StepperMonitor_h


#include <SensorBase.h>
#include <str.h>

class StepperActuator;
class StepperActuator2;


class StepperMonitor : public SensorBase {
 public:
    StepperMonitor(const HiveConfig &config,
		   const char *name,
		   const class RateProvider &rateProvider,
		   const class TimeProvider &timeProvider,
		   unsigned long now,
		   const StepperActuator *actuator,
		   Mutex *wifiMutex);
    StepperMonitor(const HiveConfig &config,
		   const char *name,
		   const class RateProvider &rateProvider,
		   const class TimeProvider &timeProvider,
		   unsigned long now,
		   const StepperActuator2 *actuator,
		   Mutex *wifiMutex);
    ~StepperMonitor();

    bool isItTimeYet(unsigned long now);
    bool loop(unsigned long now);
    
    bool sensorSample(Str *value);
    
    bool processResult(const HttpCouchConsumer &consumer, unsigned long *callMeBackIn_ms);
    
 private:
    const char *className() const {return "StepperMonitor";}

    const StepperActuator *mActuator;
    const StepperActuator2 *mActuator2;
    int mPrevTarget;
    Str mSensorValue;
    unsigned long mNextAction;
    bool mDoPost;
};


#endif
