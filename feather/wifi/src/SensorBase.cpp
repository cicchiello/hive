#include <SensorBase.h>

#include <Arduino.h>


#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <Mutex.h>

#include <hiveconfig.h>
#include <hive_platform.h>
#include <http_couchpost.h>
#include <RateProvider.h>
#include <TimeProvider.h>

#include <str.h>
#include <strbuf.h>


#define FIRST_SAMPLE (15*1000l)
#define FIRST_POST (30*1000l)
#define DELTA (getRateProvider().secondsBetweenSamples()*1000l)
//#define DELTA (30000l)


/* STATIC */
const Str SensorBase::UNDEF("NAN"); // used to indicate that SensorBase should not POST


SensorBase::SensorBase(const HiveConfig &config,
		       const char *name,
		       const class RateProvider &rateProvider,
		       const class TimeProvider &timeProvider,
		       unsigned long now, Mutex *wifiMutex)
  : Sensor(name, rateProvider, timeProvider, now), mValueStr(new Str(UNDEF)),
    mPoster(0), mConfig(config), mWifiMutex(wifiMutex)
{
    setNextSampleTime(now + FIRST_SAMPLE);
    mNextPostTime = now + FIRST_POST;
}


SensorBase::~SensorBase()
{
    delete mValueStr;
}


bool SensorBase::processResult(const HttpCouchConsumer &consumer, unsigned long *callMeBackIn_ms)
{
    TF("SensorBase::processResult");
    if (consumer.hasOk()) {
        TRACE("POST succeeded");
    } else {
        PH2("fail POST; response: ", consumer.getResponse().c_str());
    }
    *callMeBackIn_ms = DELTA;
    return false;
}


bool SensorBase::postImplementation(unsigned long now, Mutex *wifi)
{
    TF("SensorBase::postImplementation");
    bool callMeBack = true;
    unsigned long callMeBackIn_ms = 10l;
    if (wifi->own(this)) {
	if (mPoster == NULL) {
	    TRACE("creating poster");

	    // curl -v -H "Content-Type: application/json" -X POST "https://afteptsecumbehisomorther:e4f286be1eef534f1cddd6240ed0133b968b1c9a@jfcenterprises.cloudant.com:443/hive-sensor-log" -d '{"hiveid":"F0-17-66-FC-5E-A1","sensor":"heartbeat","timestamp":"1485894682","value":"0.9.104"}'
  
	    Str timestampStr;
	    getTimeProvider().toString(now, &timestampStr);

	    CouchUtils::Doc doc;
	    doc.addNameValue(new CouchUtils::NameValuePair("hiveid",
							   CouchUtils::Item(mConfig.getHiveId())));
	    doc.addNameValue(new CouchUtils::NameValuePair("sensor",
							   CouchUtils::Item(Str(getName()))));
	    doc.addNameValue(new CouchUtils::NameValuePair("timestamp",
							   CouchUtils::Item(timestampStr)));
	    doc.addNameValue(new CouchUtils::NameValuePair("value",
							   CouchUtils::Item(*mValueStr)));
	    StrBuf dump;
	    CouchUtils::toString(doc, &dump);

	    StrBuf url("/");
	    url.append(mConfig.getLogDbName().c_str());
	    
	    TRACE2("creating POST with url: ", url.c_str());
	    TRACE2("thru wifi: ", mConfig.getSSID().c_str());
	    TRACE2("with pswd: ", mConfig.getPSWD().c_str());
	    TRACE2("to host: ", mConfig.getDbHost().c_str());
	    TRACE2("port: ", mConfig.getDbPort());
	    TRACE2("using ssl? ", (mConfig.isSSL() ? "yes" : "no"));
	    TRACE2("with dbuser: ", mConfig.getDbUser().c_str());
	    TRACE2("with dbpswd: ", mConfig.getDbPswd().c_str());
	    PH2("POSTing doc: ", dump.c_str());
	    
	    mPoster = new HttpCouchPost(mConfig.getSSID(), mConfig.getPSWD(),
					mConfig.getDbHost(), mConfig.getDbPort(),
					url.c_str(), doc,
					mConfig.getDbUser(), mConfig.getDbPswd(), 
					mConfig.isSSL());
	} else {
	    HivePlatform::nonConstSingleton()->clearWDT();
	    HivePlatform::singleton()->markWDT("processing events");
	    HttpCouchPost::EventResult er = mPoster->event(now, &callMeBackIn_ms);
	    if (!mPoster->processEventResult(er)) {
	        bool retry = true;
		callMeBackIn_ms = 5000l;
		if (mPoster->getHeaderConsumer().hasOk()) {
		    callMeBack = processResult(mPoster->getCouchConsumer(), &callMeBackIn_ms);
		    retry = false;
		} else if (mPoster->isTimeout()) {
		    PH("timeout; POST failed");
		} else if (mPoster->isError()) {
		    PH("error; POST failed");
		} else {
		    PH("POST failed for unknown reasons; retrying again in 5s");
		}

		mPoster->shutdownWifiOnDestruction(retry);
		delete mPoster;
		mPoster = NULL;
		wifi->release(this);
	    }
	}
    }
    mNextPostTime = now + callMeBackIn_ms;

    return callMeBack;
}


void SensorBase::setSample(const Str &value)
{
    *mValueStr = value;
}


bool SensorBase::loop(unsigned long now)
{
    TF("SensorBase::loop");
    
    if (now >= getNextSampleTime()) {
        sensorSample(mValueStr);
        TRACE4("sampled sensor ", getName(), ": ", mValueStr->c_str());
	setNextSampleTime(now + DELTA);
    }

    const char *value = NULL;
    bool callMeBack = true; // normally want to be called back -- at some later pre-determined time
    if ((now >= mNextPostTime) && !mValueStr->equals(UNDEF)) {
        //TRACE2("calling postImplementation for: ", getName());
        // let the postImplementation say whether we're totally done or not
        callMeBack = postImplementation(now, mWifiMutex);
    }
    
    return callMeBack;
}


