#include <SensorBase.h>

#include <Arduino.h>


#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <Mutex.h>

#include <hiveconfig.h>
#include <http_couchpost.h>

#include <RateProvider.h>
#include <TimeProvider.h>

#include <str.h>


SensorBase::SensorBase(const HiveConfig &config,
		       const char *name,
		       const class RateProvider &rateProvider,
		       const class TimeProvider &timeProvider,
		       unsigned long now)
  : Sensor(name, rateProvider, timeProvider, now), mValueStr(new Str("NAN")),
    mPoster(0), mConfig(config)
{
    setNextTime(now, &mNextSampleTime);
    setNextTime(now, &mNextPostTime);
}


SensorBase::~SensorBase()
{
    delete mValueStr;
}


/* STATIC */
void SensorBase::setNextTime(unsigned long now, unsigned long *t)
{
    *t = now + 30000l /*rateProvider.secondsBetweenSamples()*1000l*/;
}


bool SensorBase::isItTimeYet(unsigned long now)
{
    return (now >= mNextSampleTime) || (now >= mNextPostTime);
}


void SensorBase::postImplementation(unsigned long now, Mutex *wifi)
{
    TF("SensorBase::postImplementation");
    if (wifi->own(this)) {
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
							   CouchUtils::Item(*mValueStr)));
	    Str dump;
	    CouchUtils::toString(doc, &dump);

	    Str url("/");
	    url.append(mConfig.getLogDbName());
	    
	    TRACE2("creating POST with url: ", url.c_str());
	    TRACE2("thru wifi: ", mConfig.getSSID());
	    TRACE2("with pswd: ", mConfig.getPSWD());
	    TRACE2("to host: ", mConfig.getDbHost());
	    TRACE2("port: ", mConfig.getDbPort());
	    TRACE2("using ssl? ", (mConfig.isSSL() ? "yes" : "no"));
	    TRACE2("with creds: ", mConfig.getDbCredentials());
	    TRACE2("doc: ", dump.c_str());
	    
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
		    setNextTime(0, &callMeBackIn_ms);
		} else {
		    TRACE("POST failed; retrying again in 5s");
		    callMeBackIn_ms = 5000l;
		}
		delete mPoster;
		mPoster = NULL;
		wifi->release(this);
	    }
	}
	mNextPostTime = now + callMeBackIn_ms;
    }
}


bool SensorBase::loop(unsigned long now, Mutex *wifi)
{
    TF("SensorBase::loop");

    if (now > mNextSampleTime) {
        sensorSample(mValueStr);
	setNextTime(now, &mNextSampleTime);
    }

    const char *value = NULL;
    if ((now >= mNextPostTime) && (strcmp(mValueStr->c_str(),"NAN")!=0)) {
        postImplementation(now, wifi);
    }
    
    return true; // always want to be called back eventually; when will be determined by mNextActionTime
}


