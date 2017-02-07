#include <StepperMonitor.h>

#include <Arduino.h>


#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <StepperActuator.h>


StepperMonitor::StepperMonitor(const HiveConfig &config,
			       const char *name,
			       const class RateProvider &rateProvider,
			       const class TimeProvider &timeProvider,
			       unsigned long now,
			       const class StepperActuator &actuator)
  : SensorBase(config, name, rateProvider, timeProvider, now),
    mActuator(actuator)
{
    TF("StepperMonitor::StepperMonitor");
    mTarget = actuator.getTarget();
}


StepperMonitor::~StepperMonitor()
{
    TF("StepperMonitor::~StepperMonitor");
}


const void *StepperMonitor::getSemaphore() const
{
    TF("StepperMonitor::getSemaphore");
    return getName();
}


bool StepperMonitor::sensorSample(Str *value)
{
    TF("StepperMonitor::sensorSample");
    
    int loc = mActuator.getLocation();
	
    char buf[10];
    *value = itoa(loc, buf, 10);
    return true;
}


bool StepperMonitor::isItTimeYet(unsigned long now)
{
    TF("StepperMonitor::isItTimeYet");
    bool baseIsItTimeYet = SensorBase::isItTimeYet(now);
    
    int currTarget = mActuator.getTarget();
    if (!baseIsItTimeYet && (mTarget != currTarget)) {
        mTarget = currTarget;
	unsigned long n = getNextSampleTime();
	if (n > now + 100l) 
	    setNextSampleTime(now + 100l); // force a sample soon (100ms) since the motor should now be running
	unsigned long p = getNextPostTime();
	if (p > now + 1000l)
	    setNextPostTime(now + 1000l);
	return false;
    } else {
        return baseIsItTimeYet;
    }
}



