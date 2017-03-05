#include <StepperMonitor.h>

#include <Arduino.h>


#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <strbuf.h>

#include <StepperActuator.h>


StepperMonitor::StepperMonitor(const HiveConfig &config,
			       const char *name,
			       const class RateProvider &rateProvider,
			       const class TimeProvider &timeProvider,
			       unsigned long now,
			       const class StepperActuator &actuator,
			       Mutex *wifiMutex)
  : SensorBase(config, name, rateProvider, timeProvider, now, wifiMutex),
    mActuator(actuator), mPrevTarget(0), mNextAction(now+1500l), mDoPost(false)
{
    TF("StepperMonitor::StepperMonitor");
    setNextSampleTime(now + 1000000000); // don't let the base class ever perform it's sample function
}


StepperMonitor::~StepperMonitor()
{
    TF("StepperMonitor::~StepperMonitor");
}


bool StepperMonitor::sensorSample(Str *value)
{
    TF("StepperMonitor::sensorSample");
    assert(false, "Shouldn't get here 'cause I've taken over sample timing and explicity set the sample val");
    return false;
}


bool StepperMonitor::isItTimeYet(unsigned long now)
{
    return (now >= mNextAction) || mDoPost;
}


bool StepperMonitor::processResult(const HttpCouchConsumer &consumer, unsigned long *callMeBackIn_ms)
{
    TF("StepperMonitor::processResult");
    mDoPost = false;
    return SensorBase::processResult(consumer, callMeBackIn_ms);
}


bool StepperMonitor::loop(unsigned long now)
{
    TF("StepperMonitor::loop");

    if (now > mNextAction || mNextAction > now +200l) {
        mNextAction = now + 200l;
	setNextSampleTime(now + 1000000000); // don't let the base class ever perform it's sample function
    }
    
    int target = mActuator.getTarget();
    if (target != 0) {
        StrBuf buf("moving");
	buf.add(target > 0 ? '+' : '-');
	buf.append(target);
        mSensorValue = buf.c_str();
    } else {
        static Str stopped("stopped");
        mSensorValue = stopped;
    }
    
    if (target != mPrevTarget) {
        setSample(mSensorValue);
	setNextPostTime(now);   // force immediate POST attempt to make the app as responsive as possible
	mDoPost = true;
	mPrevTarget = target;
    }
      
    bool callMeBack = true;
    if (mDoPost) {
        callMeBack = SensorBase::loop(now);
    }

    return callMeBack;
}


