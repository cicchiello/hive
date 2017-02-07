#include <TempSensor.h>

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

#include <DHT.h>

#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
#define DHTPIN A3       // what pin we're connected to for data

static DHT dht(DHTPIN, DHTTYPE);
static unsigned long lastSampleTime = 0;

static void *MySemaphore = &MySemaphore; // used by mutex


static void setNextTime(unsigned long now, unsigned long *t)
{
    *t = now + 30000l /*rateProvider.secondsBetweenSamples()*1000l*/;
}




TempSensor::TempSensor(const HiveConfig &config,
		       const char *name,
		       const class RateProvider &rateProvider,
		       const class TimeProvider &timeProvider,
		       unsigned long now)
  : Sensor(name, rateProvider, timeProvider, now), mTempStr(new Str("NAN")), mT(25.0), mHasTemp(false),
    mPoster(0), mConfig(config)
{
    setNextTime(now, &mNextSampleTime);
    setNextTime(now, &mNextPostTime);
    dht.begin();
}


TempSensor::~TempSensor()
{
    delete mTempStr;
}


/* STATIC */
DHT &TempSensor::getDht()
{
    return dht;
}

bool TempSensor::sensorSample(Str *value)
{
    double t = dht.readTemperature();
    if (isnan(t)) {
        *value = Str("NAN");
	D("Providing stale temp: ");
	DL(value->c_str());
    } else {
        mT = t;
	mHasTemp = true;

	int degrees = (int) t;
	int hundredths = (int) (100*(t - ((double) degrees)));
	    
	char degreesStr[20];
	sprintf(degreesStr, "%d", degrees);
	
	char hundredthsStr[20];
	sprintf(hundredthsStr, "%d", hundredths);

	char output[20];
	sprintf(output, "%s.%s", degreesStr, hundredthsStr);
	    
	*value = output;
    
	D("Measured ambient temp: ");
	DL(output);
    }
}


bool TempSensor::isItTimeYet(unsigned long now)
{
    return (now >= mNextSampleTime) || (now >= mNextPostTime);
}


bool TempSensor::loop(unsigned long now, Mutex *wifi)
{
    TF("TempSensor::loop");

    if (now > mNextSampleTime) {
        sensorSample(mTempStr);
	setNextTime(now, &mNextSampleTime);
    }
    
    if ((now >= mNextPostTime) && (strcmp(mTempStr->c_str(),"NAN")!=0) && wifi->own(MySemaphore)) {
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
							   CouchUtils::Item(*mTempStr)));
	    CouchUtils::printDoc(doc);

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
		    setNextTime(0, &callMeBackIn_ms);
		} else {
		    TRACE("POST failed; retrying again in 5s");
		    callMeBackIn_ms = 5000l;
		}
		delete mPoster;
		mPoster = NULL;
		wifi->release(MySemaphore);
	    }
	}
	mNextPostTime = now + callMeBackIn_ms;
    }
    
    return true; // always want to be called back eventually; when will be determined by mNextActionTime
}


