#include <StepperMonitor.h>

#include <Arduino.h>


#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <strbuf.h>

#include <StepperActuator.h>
#include <StepperActuator2.h>


StepperMonitor::StepperMonitor(const HiveConfig &config,
			       const char *name,
			       const class RateProvider &rateProvider,
			       unsigned long now,
			       const class StepperActuator *actuator,
			       Mutex *wifiMutex)
  : SensorBase(config, name, rateProvider, now, wifiMutex),
    mActuator2(0), mActuator(actuator), mPrevTarget(-1), mNextAction(now+1500l), mDoPost(false)
{
    TF("StepperMonitor::StepperMonitor");
    setNextSampleTime(now + 1000000000); // don't let the base class ever perform it's sample function
}


StepperMonitor::StepperMonitor(const HiveConfig &config,
			       const char *name,
			       const class RateProvider &rateProvider,
			       unsigned long now,
			       const class StepperActuator2 *actuator,
			       Mutex *wifiMutex)
  : SensorBase(config, name, rateProvider, now, wifiMutex),
    mActuator2(actuator), mActuator(0), mPrevTarget(-1), mNextAction(now+1500l), mDoPost(false)
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


bool StepperMonitor::processResult(const HttpCouchConsumer &consumer, unsigned long *callMeBackIn_ms,
				   bool *keepMutex, bool *success)
{
    TF("StepperMonitor::processResult");
    mDoPost = false;
    return SensorBase::processResult(consumer, callMeBackIn_ms, keepMutex, success);
}


bool StepperMonitor::loop(unsigned long now)
{
    TF("StepperMonitor::loop");

    if (now > mNextAction || mNextAction > now +200l) {
        mNextAction = now + 200l;
	setNextSampleTime(now + 1000000); // don't let the base class ever perform it's sample function
    }
    
    int target = mActuator ? mActuator->getTarget() : mActuator2->getTarget();
    if (target != 0) {
        StrBuf buf("moving");
	if (target>0) // '-' will be added automatically by virtue of the sign
	    buf.add('+');
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


