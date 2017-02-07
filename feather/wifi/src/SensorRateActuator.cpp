#include <SensorRateActuator.h>

#include <Arduino.h>


//#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <Mutex.h>


#include <platformutils.h>
#include <hiveconfig.h>

#include <str.h>

#include <http_couchget.h>

static void *MySemaphore = &MySemaphore; // used by mutex


class SensorRateGetter : public HttpCouchGet {
private:
    bool mHasRate;
    int mRate;
  
public:
    SensorRateGetter(const char *ssid, const char *pswd, const char *dbHost, int dbPort, bool isSSL,
		     const char *url, const char *credentials)
      : HttpCouchGet(ssid, pswd, dbHost, dbPort, url, credentials, isSSL)
    {
        TF("SensorRateGetter::SensorRateGetter");
	TRACE("entry");
    }
 
    int getRate() const {
        TF("SensorRateGetter::getRate");
        assert(mHasRate, "mHasRate");
	return mRate;
    }

    bool hasRate() const {
        TF("SensorRateGetter::hasRate");
	if (m_consumer.hasOk()) {
	    TRACE(m_consumer.getResponse().c_str());

	    const char *jsonStr = strstr(m_consumer.getResponse().c_str(), "total_rows");
	    if (jsonStr != NULL) {
                SensorRateGetter *nonConstThis = (SensorRateGetter*)this;
		nonConstThis->mHasRate = false;
	        // work back to the beginning of the doc... then some ugly parsing!
	        while ((jsonStr > m_consumer.getResponse().c_str()) && (*jsonStr != '{'))
		    --jsonStr;

		CouchUtils::Doc doc;
		const char *remainder = CouchUtils::parseDoc(jsonStr, &doc);
		if (remainder != NULL) {
		    int ir = doc.lookup("rows");
		    if ((ir>=0) && doc[ir].getValue().isArr()) {
                        const CouchUtils::Arr &rows = doc[ir].getValue().getArr();
			if ((rows.getSz() == 1) && rows[0].isDoc()) {
			    const CouchUtils::Doc &record = rows[0].getDoc();
			    int ival = record.lookup("value");
			    if ((ival >= 0) &&
				record[ival].getValue().isArr() &&
				(record[ival].getValue().getArr().getSz()==4)) {
                                const CouchUtils::Arr &val = record[ival].getValue().getArr();
				if (!val[3].isDoc() && !val[3].isArr()) {
				    const Str &rateStr = val[3].getStr();
				    nonConstThis->mRate = atoi(rateStr.c_str());
				    nonConstThis->mHasRate = true;
				    return true;
				}
			    }
			}
		    }
		}
	    }
	}
	
	return false;
    }
};



SensorRateActuator::SensorRateActuator(const HiveConfig &config, const char *name, unsigned long now)
  : Actuator(name, now+10*1000), mSeconds(5*60),
    mConfig(config), mGetter(NULL)
{
    // schedule first sample time
    mNextActionTime = now + 7*1000; // base class will set to 5s; use 7s to have this running off sync of others
}


bool SensorRateActuator::loop(unsigned long now, Mutex *wifi)
{
    TF("SensorRateActuator::loop");
    if (wifi->own(MySemaphore)) {
        if (mGetter == NULL) {
	    TRACE("creating getter");

	    // curl -X GET 'http://jfcenterprises.cloudant.com/hive-sensor-log/_design/SensorLog/_view/by-hive-sensor?endkey=%5B%22F0-17-66-FC-5E-A1%22,%22sample-rate%22,%2200000000%22%5D&startkey=%5B%22F0-17-66-FC-5E-A1%22,%22sensor-rate%22,%2299999999%22%5D&descending=true&limit=1'
	  
	    Str url, encodedUrl;
	    url.append(mConfig.getDesignDocId());
	    url.append("/_view/");
	    url.append(mConfig.getSensorByHiveViewName());
	    url.append("?endkey=[\"");
	    url.append(mConfig.getHiveId());
	    url.append("\",\"sample-rate\",\"00000000\"]&startkey=[\"");
	    url.append(mConfig.getHiveId());
	    url.append("\",\"sample-rate\",\"99999999\"]&descending=true&limit=1");
	    CouchUtils::urlEncode(url.c_str(), &encodedUrl);
	
	    url.clear();
	    CouchUtils::toURL(mConfig.getLogDbName(), encodedUrl.c_str(), &url);
	    TRACE2("URL: ", url.c_str());
	
	    mGetter = new SensorRateGetter(mConfig.getSSID(), mConfig.getPSWD(),
					   mConfig.getDbHost(), mConfig.getDbPort(), mConfig.isSSL(),
					   url.c_str(), mConfig.getDbCredentials());
	} else {
	    TRACE("processing event");
	    unsigned long callMeBackIn_ms = 0;
	    if (!mGetter->processEventResult(mGetter->event(now, &callMeBackIn_ms))) {
	        TRACE("done");
		bool retry = false;
		if (mGetter->hasRate()) {
		    mSeconds = mGetter->getRate();
		    P("SensorSampleRate (s): "); PL(mSeconds);
		    callMeBackIn_ms = mSeconds*1000l; // schedule revisit in a while
		} else {
		    TRACE("Rate not found in the response; retrying again in 5s");
		    retry = true;
		}
		if (retry) {
		    TRACE("setting up for a retry");
		    callMeBackIn_ms = 5000l;
		}
		delete mGetter;
		mGetter = NULL;
		wifi->release(MySemaphore);
	    }
	    mNextActionTime = now + callMeBackIn_ms;
	}
    }
    
    return true; // always want to be called back eventually; when will be determined by mNextActionTime
}




