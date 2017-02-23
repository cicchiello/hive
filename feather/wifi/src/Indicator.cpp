#include <Indicator.h>

#include <Arduino.h>


#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <Mutex.h>

#include <hiveconfig.h>

#include <http_couchpost.h>


Indicator::Indicator(unsigned long now) 
  : mLedIsOn(false), mNextActionTime(now + LED_TRYING_TOGGLE_RATE_MS), mFlashModeChanged(true),
    mFlashMode(TryingToConnect)
{
    TF("Indicator::Indicator");
    pinMode(LED_PIN, OUTPUT);  // pin will be used to indicate heartbeat
    TRACE2("now: ", now);
    TRACE2("mNextActionTime: ", mNextActionTime);
}


Indicator::~Indicator()
{
}


void Indicator::considerFlash(unsigned long now, unsigned long rate_ms, unsigned long onTime_ms)
{
    TF("Indicator::considerFlash");
    if (now >= mNextActionTime) {
        //TRACE3("Turning LED ", (mLedIsOn ? "off " : "on "), now);
	mNextActionTime = now + (mLedIsOn ? rate_ms - onTime_ms : onTime_ms);
	digitalWrite(LED_PIN, mLedIsOn ? LOW : HIGH);
	mLedIsOn = !mLedIsOn;
    }
}


Indicator::FlashMode Indicator::setFlashMode(Indicator::FlashMode m)
{
    TF("Indicator::loop");
    
    FlashMode r = mFlashMode;
    if (r != m) {
        TRACE2("changing to mode: ", m);
        mFlashMode = m;
	mFlashModeChanged = true;
    }
    return r;
}


bool Indicator::loop(unsigned long now)
{
    TF("Indicator::loop");

    if (mFlashModeChanged || (now > mNextActionTime)) {
      
        // blink LED
        switch (mFlashMode) {
	case TryingToConnect: considerFlash(now, LED_TRYING_TOGGLE_RATE_MS, LED_TRYING_ON_TIME_MS); break;
	case Provisioning: considerFlash(now, LED_PROVISION_TOGGLE_RATE_MS, LED_PROVISION_ON_TIME_MS); break;
	case Normal: considerFlash(now, LED_NORMAL_TOGGLE_RATE_MS, LED_NORMAL_ON_TIME_MS); break;
	case Error: considerFlash(now, LED_ERROR_TOGGLE_RATE_MS, LED_ERROR_ON_TIME_MS); break;
	default: TRACE2("Unknown mFlashMode", mFlashMode);
	}
	
	mFlashModeChanged = false;
    }

    return true; // always want to be called back eventually; when will be determined by mNextActionTime
}

