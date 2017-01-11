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


StepperMonitor::StepperMonitor(const StepperActuator &actuator, unsigned long now)
  : Sensor(actuator.getName(), now), mActuator(actuator), mPrev(new Str("NAN"))
{
    DL("StepperMonitor::StepperMonitor called");
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


void StepperMonitor::scheduleNextSample(unsigned long now)
{
    if (mActuator.getLocation() != mActuator.getTarget())
        mNextSampleTime = now + 5*1000;
    else
        Sensor::scheduleNextSample(now);
}


