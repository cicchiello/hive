#include <StepperMonitor.h>

#include <Arduino.h>


#define HEADLESS
#define NDEBUG

#ifndef HEADLESS
#define P(args) Serial.print(args)
#define PL(args) Serial.println(args)
#else
#define P(args) 
#define PL(args) 
#endif


#ifndef NDEBUG
#define D(args) P(args)
#define DL(args) PL(args)
#else
#define D(args)
#define DL(args)
#endif

#include <StepperActuator.h>

#include <str.h>


StepperMonitor::StepperMonitor(const StepperActuator &actuator,
			       const class SensorRateActuator &rateProvider,
			       unsigned long now)
: Sensor(actuator.getName(), rateProvider, now), mActuator(actuator), mPrev(new Str("NAN"))
{
    DL("StepperMonitor::StepperMonitor called");
    mTarget = actuator.getTarget();
}


StepperMonitor::~StepperMonitor()
{
    DL("StepperMonitor::~StepperMonitor called");
    
    delete mPrev;
}


bool StepperMonitor::sensorSample(Str *value)
{
    unsigned long now = millis();
    PL("StepperMonitor::sensorSample called");

    int loc = mActuator.getLocation();
	
    char buf[10];
    *value = itoa(loc, buf, 10);
    *mPrev = *value;
}


bool StepperMonitor::isItTimeYet(unsigned long now)
{
    int currTarget = mActuator.getTarget();
    if (mTarget != currTarget) {
        DL("StepperMonitor::isItTimeYet; determined that motor is moving");
	mTarget = currTarget;
        mNextSampleTime = now + 100; // force a sample soon (100ms) since the motor should now be running
	return false;
    } else {
        return Sensor::isItTimeYet(now);
    }
}


void StepperMonitor::scheduleNextSample(unsigned long now)
{
    if (mActuator.getLocation() != mActuator.getTarget())
        mNextSampleTime = now + 2*1000;
    else
        Sensor::scheduleNextSample(now);
}


