#include <Heartbeat.h>

#include <Arduino.h>


#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <Mutex.h>

#include <RateProvider.h>
#include <TimeProvider.h>

#include <hiveconfig.h>

#include <http_couchpost.h>


HeartBeat::HeartBeat(const HiveConfig &config,
		     const char *name,
		     const class RateProvider &rateProvider,
		     const class TimeProvider &timeProvider,
		     unsigned long now)
  : SensorBase(config, name, rateProvider, timeProvider, now), 
    mLedIsOn(false), mNextActionTime(now + LED_OFFLINE_TOGGLE_RATE_MS), mFlashModeChanged(true),
    mFlashMode(Initial), mNextBlinkActionTime(0), mTimestampStr(new Str("1470000000")),
    mNextPostActionTime(now + 30000l /*rateProvider.secondsBetweenSamples()*1000l*/)
{
    TF("HeartBeat::HeartBeat");
    pinMode(LED_PIN, OUTPUT);  // pin will be used to indicate heartbeat
    TRACE2("now: ", now);
    TRACE2("mNextPostActionTime: ", mNextPostActionTime);
}


HeartBeat::~HeartBeat()
{
    delete mTimestampStr;
}


bool HeartBeat::isItTimeYet(unsigned long now)
{
    return mFlashModeChanged || (now >= mNextActionTime);
}


void HeartBeat::considerFlash(unsigned long now, unsigned long rate_ms, unsigned long onTime_ms)
{
    TF("HeartBeat::considerFlash");
    if (now >= mNextBlinkActionTime) {
        //TRACE2("Turning LED ", mLedIsOn ? "off" : "on");
	mNextBlinkActionTime = now + (mLedIsOn ? rate_ms - onTime_ms : onTime_ms);
	digitalWrite(LED_PIN, mLedIsOn ? LOW : HIGH);
	mLedIsOn = !mLedIsOn;
    }
}


bool HeartBeat::sensorSample(Str *valueStr)
{
    *valueStr = *mTimestampStr;
    return true;
}


bool HeartBeat::loop(unsigned long now, Mutex *wifi)
{
    TF("HeartBeat::loop");

    // blink LED
    switch (mFlashMode) {
    case Initial: {
        considerFlash(now, LED_OFFLINE_TOGGLE_RATE_MS, LED_OFFLINE_ON_TIME_MS);
	if (getTimeProvider().haveTimestamp()) {
	    setFlashMode(Normal);
	    mNextPostActionTime = now + 30000l /*getRateProvider().secondsBetweenSamples()*1000l*/;
	}
    }
      break;
    case Offline: considerFlash(now, LED_OFFLINE_TOGGLE_RATE_MS, LED_OFFLINE_ON_TIME_MS); break;
    case Normal: considerFlash(now, LED_NORMAL_TOGGLE_RATE_MS, LED_NORMAL_ON_TIME_MS); break;
    case Error: considerFlash(now, LED_ERROR_TOGGLE_RATE_MS, LED_ERROR_ON_TIME_MS); break;
    default: TRACE2("Unknown mFlashMode", mFlashMode);
    }

    mNextActionTime = mNextBlinkActionTime;
    
    mFlashModeChanged = false;

    if (mFlashMode == Normal) {
        if (now >= mNextPostActionTime) {
	    getTimeProvider().toString(now, mTimestampStr);
	    
	    postImplementation(now, wifi);
	}
    }
    
    return true; // always want to be called back eventually; when will be determined by mNextActionTime
}

