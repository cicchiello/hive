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
  : Sensor(now), mActuator(actuator), mPrev(new Str("NAN"))
{
    DL("StepperMonitor::StepperMonitor called");
}


StepperMonitor::~StepperMonitor()
{
    DL("StepperMonitor::~StepperMonitor called");
    
    delete mPrev;
}


void StepperMonitor::enqueueRequest(const char *value, const char *timestamp)
{
    enqueueFullRequest(mActuator.getName(), value, timestamp);
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


