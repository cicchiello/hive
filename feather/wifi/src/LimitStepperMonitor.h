#ifndef LimitStepperMonitor_h
#define LimitStepperMonitor_h


#include <SensorBase.h>
#include <str.h>

class LimitStepperActuator;

class LimitStepperMonitor : public SensorBase {
 public:
    LimitStepperMonitor(const HiveConfig &config,
		   const char *name,
		   const class RateProvider &rateProvider,
		   unsigned long now,
		   const LimitStepperActuator *actuator,
		   Mutex *wifiMutex);

    bool isItTimeYet(unsigned long now);
    bool loop(unsigned long now);
    
    bool sensorSample(Str *value);
    
    bool processResult(const HttpCouchConsumer &consumer, unsigned long *callMeBackIn_ms,
		       bool *keepMutex, bool *success);

 private:
    const char *className() const {return "StepperMonitor";}

    const LimitStepperActuator &mActuator;
    Str mPrevSensorValue;
    unsigned long mNextAction;
    bool mDoPost;
};



#endif
