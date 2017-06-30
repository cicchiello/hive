#include <LimitStepperMonitor.h>

#include <Arduino.h>


#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <strbuf.h>

#include <LimitStepperActuator.h>


LimitStepperMonitor::LimitStepperMonitor(const HiveConfig &config,
					 const char *name,
					 const class RateProvider &rateProvider,
					 unsigned long now,
					 const class LimitStepperActuator *actuator,
					 Mutex *wifiMutex)
  : SensorBase(config, name, rateProvider, now, wifiMutex),
    mActuator(*actuator), mNextAction(now+1500l), mDoPost(false), mPrevSensorValue("unknown")
{
    TF("LimitStepperMonitor::LimitStepperMonitor");
    setNextSampleTime(now + 1000000000); // don't let the base class ever perform it's sample function
}


bool LimitStepperMonitor::sensorSample(Str *value)
{
    TF("LimitStepperMonitor::sensorSample");
    assert(false, "Shouldn't get here 'cause I've taken over sample timing and explicity set the sample val");
    return false;
}


bool LimitStepperMonitor::isItTimeYet(unsigned long now)
{
    TF("LimitStepperMonitor::isItTimeYet");
    bool itIsTime = (now >= mNextAction);
    return itIsTime;
}


bool LimitStepperMonitor::processResult(const HttpJSONConsumer &consumer, unsigned long *callMeBackIn_ms,
					bool *keepMutex, bool *success)
{
    TF("LimitStepperMonitor::processResult");
    mDoPost = false;
    return SensorBase::processResult(consumer, callMeBackIn_ms, keepMutex, success);
}


bool LimitStepperMonitor::loop(unsigned long now)
{
    TF("LimitStepperMonitor::loop");

    mNextAction = now + (mDoPost ? 10l : 200l);
    setNextSampleTime(now + 1000000); // don't let the base class ever perform it's sample function

    if (!mDoPost) {
        Str sensorValue;
	switch (mActuator.getState()) {
	case LimitStepperActuator::Stopped: {
	    static Str stopped("stopped");
	    sensorValue = stopped;
	}; break;
	case LimitStepperActuator::MovingPositive: {
	    StrBuf buf("moving+");
	    buf.append(mActuator.getTarget());
	    sensorValue = buf.c_str();
	}; break;
	case LimitStepperActuator::MovingNegative: {
	    StrBuf buf("moving"); // '-' will be added automatically by virtue of the sign
	    buf.append(mActuator.getTarget());
	    sensorValue = buf.c_str();
	}; break;
	case LimitStepperActuator::AtPositiveLimit: {
	    static Str posLimit("@pos-limit");
	    sensorValue = posLimit;
	}; break;
	case LimitStepperActuator::AtNegativeLimit: {
	    static Str negLimit("@neg-limit");
	    sensorValue = negLimit;
	}; break;
	default: assert(0, "Invalid state");
	}

	if (!sensorValue.equals(mPrevSensorValue)) {
	    setSample(sensorValue);
	    setNextPostTime(now);   // force immediate POST attempt to make the app as responsive as possible
	    mDoPost = true;
	    mNextAction = millis()+10l;
	    mPrevSensorValue = sensorValue;
	    PH2("setting mPrevSensorValue to: ", sensorValue.c_str());
	}
    }

    bool callMeBack = true;
    if (mDoPost) {
        callMeBack = SensorBase::loop(now);
    }

    return callMeBack;
}


