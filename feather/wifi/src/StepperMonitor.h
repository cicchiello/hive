#ifndef StepperMonitor_h
#define StepperMonitor_h


#include <SensorBase.h>


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

    bool sensorSample(Str *value);
    
    bool isItTimeYet(unsigned long now);
    
 private:
    const char *className() const {return "StepperMonitor";}

    int mTarget;
    const class StepperActuator &mActuator;
};


#endif
