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

static void *MySemaphore = &MySemaphore; // used by mutex


HeartBeat::HeartBeat(const HiveConfig &config,
		     const char *name,
		     const class RateProvider &rateProvider,
		     const class TimeProvider &timeProvider,
		     unsigned long now)
  : Sensor(name, rateProvider, timeProvider, now), mPoster(0), mConfig(config),
    mLedIsOn(false), mNextActionTime(now + LED_OFFLINE_TOGGLE_RATE_MS), mFlashModeChanged(true),
    mFlashMode(Initial), mNextBlinkActionTime(0),
    mNextPostActionTime(now + 30000l /*rateProvider.secondsBetweenSamples()*1000l*/)
{
    TF("HeartBeat::HeartBeat");
    pinMode(LED_PIN, OUTPUT);  // pin will be used to indicate heartbeat
    TRACE2("now: ", now);
    TRACE2("mNextPostActionTime: ", mNextPostActionTime);
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
        if ((now >= mNextPostActionTime) && wifi->own(MySemaphore)) {
	    TRACE("working on posting...");
	    unsigned long callMeBackIn_ms = 10l;
	    if (mPoster == NULL) {
	        TRACE("creating poster");

		// curl -v -H "Content-Type: application/json" -X POST "https://afteptsecumbehisomorther:e4f286be1eef534f1cddd6240ed0133b968b1c9a@jfcenterprises.cloudant.com:443/hive-sensor-log" -d '{"hiveid":"F0-17-66-FC-5E-A1","sensor":"heartbeat","timestamp":"1485894682","value":"0.9.104"}'
  
		Str timestampStr;
		getTimeProvider().toString(now, &timestampStr);

		CouchUtils::Doc doc;
		doc.addNameValue(new CouchUtils::NameValuePair("hiveid",
							       CouchUtils::Item(Str(mConfig.getHiveId()))));
		doc.addNameValue(new CouchUtils::NameValuePair("sensor",
							       CouchUtils::Item(Str(getName()))));
		doc.addNameValue(new CouchUtils::NameValuePair("timestamp",
							       CouchUtils::Item(timestampStr)));
		doc.addNameValue(new CouchUtils::NameValuePair("value",
							       CouchUtils::Item(Str(mConfig.getVersionId()))));
		//CouchUtils::printDoc(doc);

		Str url("/");
		url.append(mConfig.getLogDbName());
	    
		mPoster = new HttpCouchPost(mConfig.getSSID(), mConfig.getPSWD(),
					    mConfig.getDbHost(), mConfig.getDbPort(),
					    url.c_str(), doc,
					    mConfig.getDbCredentials(),
					    mConfig.isSSL());
	    } else {
	        TRACE("processing event");
		HttpCouchPost::EventResult er = mPoster->event(now, &callMeBackIn_ms);
		TRACE2("event result: ", er);
		if (!mPoster->processEventResult(er)) {
		    TRACE("done");
		    if (mPoster->getHeaderConsumer().hasOk()) {
		        TRACE("hasOk");
			callMeBackIn_ms = 30000l /*getRateProvider().secondsBetweenSamples()*1000l*/;
		    } else {
		        TRACE("POST failed; retrying again in 5s");
			callMeBackIn_ms = 5000l;
		    }
		    delete mPoster;
		    mPoster = NULL;
		    wifi->release(MySemaphore);
		}
	    }
	    mNextPostActionTime = now + callMeBackIn_ms;
	}

        mNextActionTime = mNextPostActionTime < mNextBlinkActionTime ? mNextPostActionTime : mNextBlinkActionTime;
    }
    
    return true; // always want to be called back eventually; when will be determined by mNextActionTime
}

